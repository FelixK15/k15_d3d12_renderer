#define NOMINMAX
#include "imgui\imgui.h"
#include "imgui\backends\imgui_impl_win32.h"
#include "imgui\backends\imgui_impl_dx12.h"

#include "..\k15_d3d12_renderer.hpp"

const char imguiVertexShader[] = R"(
struct VertexInput
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput output;
    output.pos = float4(vertexInput.pos.x, vertexInput.pos.y, 0.0f, 1.0f);
    output.uv = vertexInput.uv;
    return output;
}
)";

const char imguiPixelShader[] = R"(
SamplerState mySampler : register(s0, space1);
Texture2D texture : register(t0, space2);

struct PixelInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PixelInput input) : SV_Target
{
    return texture.Sample(mySampler, input.uv);
}
)";

struct sample_imgui_state_t
{
    bool mainWindowOpen;
    bool mainWindowMaximized;
    bool mainWindowMinimized;
    bool mainWindowIsMaximized;
    bool showSamplesMenu;
};

struct sample_frame_parameter_t
{
    HWND                    pWindowHandle;
	memory_allocator_t*	    pAllocator;
    render_context_t*       pRenderContext;
	graphics_frame_t* 	    pGraphicsFrame;
	render_target_t* 	    pRenderTarget;

    sample_imgui_state_t*   pImGuiState;
    uint32_t*               pActiveSampleIndex;
	void* 				    pUserData;

	float 				    deltaTimeInMs;
    float                   totalFrameTimeInMs;
	uint32_t 			    frameIndex;
    uint32_t                windowWidth;
    uint32_t                windowHeight;
};

#include "test_base.hpp"
#include "clear_backbuffer\clear_backbuffer.cpp"
#include "render_triangle\render_triangle.cpp"
#include "spinning_cube\spinning_cube.cpp"
#include "compute_texture\compute_texture.cpp"
#include "sponza\sponza.cpp"
#pragma comment(lib, "Advapi32.lib")

descriptor_heap_t* pImGuiDescriptorHeap = nullptr;

enum sample_type_t : uint8_t
{
    clear_background,
    render_triangle,
    spinning_cube,
    compute_texture_sample,
    sponza,

    sample_count
};

const char* pSampleNames[] = {
    "Clear Background",
    "Render Triangle",
    "Spinning Cube",
    "Compute Texture",
    "Sponza"
};
static_assert(ARRAY_SIZE(pSampleNames) == sample_type_t::sample_count);

struct sample_context_t
{
    HWND                    pWindowHandle;

    memory_allocator_t      allocator;
    render_context_t*       pRenderContext;
    sample_imgui_state_t    imguiState;

    graphics_pipeline_t*    pSampleRenderGraphicsPipeline;
    vertex_format_t*        pSampleRenderQuadVertexFormat;
    texture_sampler_t*      pSampler;
    gpu_buffer_t*           pSampleRenderQuadVertexBuffer;
    gpu_texture_t*          pSampleRenderTargetTexture;
    render_target_t*        pSampleRenderTarget;

    void*                   pUserData;

    LARGE_INTEGER           performanceFrequency;

    float 				    deltaTimeInMs;
	float 				    totalFrameTimeInMs;

    uint32_t                newWindowWidth;
    uint32_t                newWindowHeight;
    uint32_t 			    windowWidth;
	uint32_t 			    windowHeight;
    uint32_t                activeSampleIndex;
	uint32_t 			    frameIndex;

    bool                    initialized;
};

struct window_parameter_t
{
    int x;
    int y;
    int width;
    int height;
};

bool readWindowParametersFromRegistry(window_parameter_t* pOutParameter)
{
    HKEY regKey;
    DWORD dataSize = sizeof(int);
    bool success = false;
    if(RegCreateKeyA(HKEY_CURRENT_USER, "SOFTWARE\\K15TECH\\D3D12RENDERER", &regKey) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegGetValueA(regKey, "", "sample_window_x", RRF_RT_DWORD, nullptr, &pOutParameter->x, &dataSize) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegGetValueA(regKey, "", "sample_window_y", RRF_RT_DWORD, nullptr, &pOutParameter->y, &dataSize) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegGetValueA(regKey, "", "sample_window_width", RRF_RT_DWORD, nullptr, &pOutParameter->width, &dataSize) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegGetValueA(regKey, "", "sample_window_height", RRF_RT_DWORD, nullptr, &pOutParameter->height, &dataSize) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    success = true;

cleanup_and_exit:
    RegCloseKey(regKey);

    return success;
}

bool readActiveSampleFromRegistry(char* pSampleNameBuffer, uint32_t sampleNameBufferSize)
{
    HKEY regKey;
    DWORD dataSize = sampleNameBufferSize;
    bool success = false;
    if(RegCreateKeyA(HKEY_CURRENT_USER, "SOFTWARE\\K15TECH\\D3D12RENDERER", &regKey) == ERROR_SUCCESS)
    {
        if(RegGetValueA(regKey, "", "active_sample", RRF_RT_REG_SZ, nullptr, pSampleNameBuffer, &dataSize) == ERROR_SUCCESS)
        {
            success = true;
        }
    }

    RegCloseKey(regKey);
    return success;
}

bool writeWindowParametersToRegistry(const window_parameter_t* pParameter)
{
    HKEY regKey;
    bool success = false;
    if(RegCreateKeyA(HKEY_CURRENT_USER, "SOFTWARE\\K15TECH\\D3D12RENDERER", &regKey) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegSetValueExA(regKey, "sample_window_x", 0, REG_DWORD, (const BYTE*)&pParameter->x, sizeof(int)) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegSetValueExA(regKey, "sample_window_y", 0, REG_DWORD, (const BYTE*)&pParameter->y, sizeof(int)) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegSetValueExA(regKey, "sample_window_width", 0, REG_DWORD, (const BYTE*)&pParameter->width, sizeof(int)) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    if(RegSetValueExA(regKey, "sample_window_height", 0, REG_DWORD, (const BYTE*)&pParameter->height, sizeof(int)) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    success = true;

cleanup_and_exit:
    RegCloseKey(regKey);

    return success;
}

bool writeActiveSampleNameToRegistry(const char* pSampleName)
{
    HKEY regKey;
    bool success = false;
    if(RegCreateKeyA(HKEY_CURRENT_USER, "SOFTWARE\\K15TECH\\D3D12RENDERER", &regKey) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    const uint32_t sampleNameLength = (uint32_t)strlen(pSampleName);
    constexpr uint32_t maxSampleNameLength = 64;
    if(sampleNameLength > maxSampleNameLength)
    {
        goto cleanup_and_exit;
    }

    if(RegSetValueExA(regKey, "active_sample", 0, REG_SZ, (const BYTE*)pSampleName, sampleNameLength) != ERROR_SUCCESS)
    {
        goto cleanup_and_exit;
    }

    success = true;
cleanup_and_exit:
    RegCloseKey(regKey);

    return success;
}

BOOL validMonitorProc(HMONITOR pMonitor, HDC pDeviceContext, LPRECT pClipRect, LPARAM pParam)
{
    BOOL* pParameterValid = (BOOL*)pParam;
    *pParameterValid = TRUE;

    return TRUE;
}

bool isValidWindowParameters(const window_parameter_t* pWindowParameter)
{
    if(pWindowParameter->height == 0 || pWindowParameter->width == 0)
    {
        return false;
    }
    
    RECT clipRect = {};
    clipRect.left = pWindowParameter->x;
    clipRect.top = pWindowParameter->y;
    clipRect.right = pWindowParameter->x + pWindowParameter->width;
    clipRect.bottom = pWindowParameter->y + pWindowParameter->height;

    BOOL parametersAreValid = false;
    EnumDisplayMonitors(nullptr, &clipRect, validMonitorProc, (LPARAM)&parametersAreValid);

    return parametersAreValid;
}

bool pumpWin32Messages()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT)
            return false;
    }

    return true;
}

void renderImGui(graphics_frame_t* pGraphicsFrame)
{
    ImGui::Render();
    render_pass_t* pImGuiRenderPass = startRenderPass(pGraphicsFrame, "ImGui Pass", pGraphicsFrame->pBackBuffer);
    clearColorRenderTarget(pImGuiRenderPass, pGraphicsFrame->pBackBuffer, 1.0f, 1.0f, 1.0f, 1.0f);
    pImGuiRenderPass->pGpuCommandBuffer->pCommandList->SetDescriptorHeaps(1u, &pImGuiDescriptorHeap->pDescriptorHeap);
    pImGuiRenderPass->pGpuCommandBuffer->pCommandList->OMSetRenderTargets(1u, &pImGuiRenderPass->currentPipelineState.pRenderTarget->colorBufferDescriptorHandle.cpuDescriptorHandle, FALSE, nullptr);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pImGuiRenderPass->pGpuCommandBuffer->pCommandList);
    endRenderPass(pGraphicsFrame, pImGuiRenderPass);
    executeRenderPass(pGraphicsFrame, pImGuiRenderPass, render_pass_execution_order_t::push_front);
}

void startImGuiFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void shutdownSample(sample_frame_parameter_t* pSampleFrameParameter, const sample_type_t sampleType)
{
    switch(sampleType)
    {
        case sample_type_t::clear_background:
            break;
        case sample_type_t::render_triangle:
            shutdownRenderTriangleSample(pSampleFrameParameter);
            break;
        case sample_type_t::spinning_cube:
            shutdownSpinningCubeSample(pSampleFrameParameter);
            break;
        case sample_type_t::compute_texture_sample:
            shutdownComputeTextureSample(pSampleFrameParameter);
            break;
        case sample_type_t::sponza:
            shutdownSponzaSample(pSampleFrameParameter);
            break;
        case 0xFF:
            break;
        default:
            ASSERT_DEBUG_UNREACHABLE_CODE();
            break;
    }
}

void initSample(sample_frame_parameter_t* pSampleFrameParameter, const sample_type_t sampleType)
{
    switch(sampleType)
    {
        case sample_type_t::clear_background:
            break;
        case sample_type_t::render_triangle:
            initRenderTriangleSample(pSampleFrameParameter);
            break;
        case sample_type_t::spinning_cube:
            initSpinningCubeSample(pSampleFrameParameter);
            break;
        case sample_type_t::compute_texture_sample:
            initComputeTextureSample(pSampleFrameParameter);
            break;
        case sample_type_t::sponza:
            initSponzaSample(pSampleFrameParameter);
            break;
        default:
            ASSERT_DEBUG_UNREACHABLE_CODE();
            break;
    }
}

void doSample(sample_frame_parameter_t* pSampleFrameParameter, const sample_type_t sampleType)
{
    switch(sampleType)
    {
        case sample_type_t::clear_background:
            doClearBackgroundSample(pSampleFrameParameter);
            break;
        case sample_type_t::render_triangle:
            doRenderTriangleSample(pSampleFrameParameter);
            break;
        case sample_type_t::spinning_cube:
            doSpinningCubeSample(pSampleFrameParameter);
            break;
        case sample_type_t::compute_texture_sample:
            doComputeTextureSample(pSampleFrameParameter);
            break;
        case sample_type_t::sponza:
            doSponzaSample(pSampleFrameParameter);
            break;
        default:
            ASSERT_DEBUG_UNREACHABLE_CODE();
            break;
    }
}

void doGeneralSampleImGuiFrame(sample_frame_parameter_t* pSampleFrameParameter)
{
    ImVec2 windowSize = {};
    windowSize.x = (float)pSampleFrameParameter->windowWidth;
    windowSize.y = (float)pSampleFrameParameter->windowHeight;
    
    sample_imgui_state_t* pImguiState = pSampleFrameParameter->pImGuiState;
    if(!ImGui::Begin("K15 D3D12 Rendering Samples", &pImguiState->mainWindowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove, &pImguiState->mainWindowMaximized, &pImguiState->mainWindowMinimized))
    {
        return;
    }
    
    ImGui::SetWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetWindowSize(windowSize, ImGuiCond_Always);

    if(ImGui::BeginTabBar("Samples"))
    {
        for(uint32_t sampleIndex = 0u; sampleIndex < sample_type_t::sample_count; ++sampleIndex)
        {
            if(ImGui::BeginTabItem(pSampleNames[sampleIndex]))
            {
                if(*pSampleFrameParameter->pActiveSampleIndex != sampleIndex)
                {
                    writeActiveSampleNameToRegistry(pSampleNames[sampleIndex]);

                    shutdownSample(pSampleFrameParameter, (sample_type_t)*pSampleFrameParameter->pActiveSampleIndex);
                    initSample(pSampleFrameParameter, (sample_type_t)sampleIndex);

                    *pSampleFrameParameter->pActiveSampleIndex = sampleIndex;
                }

                doSample(pSampleFrameParameter, (sample_type_t)sampleIndex);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void renderSampleRenderTarget(sample_context_t* pSampleContext, graphics_frame_t* pGraphicsFrame)
{
    render_pass_t* pSampleRenderPass = startRenderPass(pGraphicsFrame, "sample rendering", pGraphicsFrame->pBackBuffer);
    setScissor(pSampleRenderPass, 0, 0, pSampleContext->windowWidth, pSampleContext->windowHeight);
    setViewport(pSampleRenderPass, 0, 0, pSampleContext->windowWidth, pSampleContext->windowHeight, 0.0f, 100.0f);
    bindVertexBuffer(pSampleRenderPass, pSampleContext->pSampleRenderQuadVertexBuffer, pSampleContext->pSampleRenderQuadVertexFormat, 0u);
    bindGraphicsPipeline(pSampleRenderPass, pSampleContext->pSampleRenderGraphicsPipeline);
    bindTextureSampler(pSampleRenderPass, pSampleContext->pSampler, 0u, 1u);
    bindTexture(pSampleRenderPass, pSampleContext->pSampleRenderTargetTexture, 0u, 2u);
    draw(pSampleRenderPass, 0u, 6u);
    endRenderPass(pGraphicsFrame, pSampleRenderPass);
    executeRenderPass(pGraphicsFrame, pSampleRenderPass);
}

void doSampleFrame(sample_context_t* pSampleContext, graphics_frame_t* pGraphicsFrame)
{
    sample_frame_parameter_t sampleFrameParameter = {};
    sampleFrameParameter.pWindowHandle      = pSampleContext->pWindowHandle;
    sampleFrameParameter.pRenderContext     = pSampleContext->pRenderContext;
    sampleFrameParameter.pActiveSampleIndex = &pSampleContext->activeSampleIndex;
    sampleFrameParameter.pGraphicsFrame     = pGraphicsFrame;
    sampleFrameParameter.deltaTimeInMs      = pSampleContext->deltaTimeInMs;
    sampleFrameParameter.frameIndex         = pSampleContext->frameIndex;
    sampleFrameParameter.windowHeight       = pSampleContext->windowHeight;
    sampleFrameParameter.windowWidth        = pSampleContext->windowWidth;
    sampleFrameParameter.pImGuiState        = &pSampleContext->imguiState;
    sampleFrameParameter.pAllocator         = &pSampleContext->allocator;
    sampleFrameParameter.pUserData          = pSampleContext->pUserData;
    sampleFrameParameter.totalFrameTimeInMs = pSampleContext->totalFrameTimeInMs;
    sampleFrameParameter.pRenderTarget      = pSampleContext->pSampleRenderTarget;

    startImGuiFrame();
    doGeneralSampleImGuiFrame(&sampleFrameParameter);
    renderImGui(pGraphicsFrame);
    renderSampleRenderTarget(pSampleContext, pGraphicsFrame);

    pSampleContext->pUserData = sampleFrameParameter.pUserData;
}

void handleMainWindowTitleBarLogic(sample_context_t* pSampleContext)
{
    if(!pSampleContext->imguiState.mainWindowOpen)
    {
        PostQuitMessage(0);
    }

    if(pSampleContext->imguiState.mainWindowMaximized)
    {
        pSampleContext->imguiState.mainWindowMaximized = false;
        
        if(pSampleContext->imguiState.mainWindowIsMaximized)
        {
            ShowWindow(pSampleContext->pWindowHandle, SW_NORMAL);
        }
        else
        {
            ShowWindow(pSampleContext->pWindowHandle, SW_MAXIMIZE);
        }
        
        pSampleContext->imguiState.mainWindowIsMaximized = !pSampleContext->imguiState.mainWindowIsMaximized;
    }

    if(pSampleContext->imguiState.mainWindowMinimized)
    {
        pSampleContext->imguiState.mainWindowMinimized = false;
        ShowWindow(pSampleContext->pWindowHandle, SW_MINIMIZE);
    }
}

void resizeSampleRenderTarget(sample_context_t* pSampleContext, graphics_frame_t* pGraphicsFrame, const float width, const float height)
{
    if(pSampleContext->pSampleRenderTargetTexture != nullptr)
    {
        releaseGpuTexture(pGraphicsFrame, pSampleContext->pSampleRenderTargetTexture);
        pSampleContext->pSampleRenderTargetTexture = nullptr;
    }

    if(pSampleContext->pSampleRenderTarget != nullptr)
    {
        releaseRenderTarget(pGraphicsFrame, pSampleContext->pSampleRenderTarget);
        pSampleContext->pSampleRenderTarget = nullptr;
    }

    const uint3_t renderTargetDimension = createUint3((uint32_t)width, (uint32_t)height, 1u);
    pSampleContext->pSampleRenderTargetTexture = createGpuTexture(pGraphicsFrame, renderTargetDimension, 0u, nullptr, gpu_texture_usage_flag_t::color_render_target | gpu_texture_usage_flag_t::shader_resource_view, gpu_texture_format_t::R8G8B8A8, gpu_texture_format_type_t::normalized_unsigned_int, "SampleRenderTarget");
    pSampleContext->pSampleRenderTarget = createRenderTarget(pGraphicsFrame, renderTargetDimension, pSampleContext->pSampleRenderTargetTexture, nullptr);
}

void createSampleRenderQuad(sample_context_t* pSampleContext, graphics_frame_t* pGraphicsFrame, const float x, const float y, const float width, const float height, const float windowWidth, const float windowHeight)
{
    if(pSampleContext->pSampleRenderQuadVertexBuffer != nullptr)
    {
        releaseGpuBuffer(pGraphicsFrame, pSampleContext->pSampleRenderQuadVertexBuffer);
        pSampleContext->pSampleRenderQuadVertexBuffer = nullptr;
    }

    const float left = 2.0f * (x / windowWidth) - 1.0f;
    const float right = 2.0f * ((x + width) / windowWidth) - 1.0f;
    const float top = 2.0f * (1.0f - y / windowHeight) - 1.0f;
    const float bottom = 2.0f * (1.0f - (y + height) / windowHeight) - 1.0f;

    const float quadVertices[] = {
        right, top,
        1.0f, 0.0f,

        right, bottom,
        1.0f, 1.0f,

        left, bottom,
        0.0f, 1.0f,

        left, bottom,
        0.0f, 1.0f,

        left, top,
        0.0f, 0.0f,

        right, top,
        1.0f, 0.0f
    };

    pSampleContext->pSampleRenderQuadVertexBuffer = createGpuBuffer(pGraphicsFrame, sizeof(quadVertices), quadVertices, gpu_buffer_usage_flag_t::vertex_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess, "SampleQuadVertices");
}

void doSampleGuiFrame(sample_context_t* pSampleContext)
{
    LARGE_INTEGER endTime, startTime;

    QueryPerformanceCounter(&startTime);
    graphics_frame_t* pGraphicsFrame = beginNextFrame(pSampleContext->pRenderContext);

    if(!pSampleContext->initialized)
    {
        vertex_attribute_entry_t vertexAttributes[] = {
            {vertex_attribute_t::position, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 2u},
            {vertex_attribute_t::texcoord, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 2u}
        };
        pSampleContext->pSampleRenderQuadVertexFormat = createVertexFormat(pGraphicsFrame, vertexAttributes, 2u);

        graphics_pipeline_parameters_t pipelineParameter = {};
        pipelineParameter.pName = "SampleQuad";
        pipelineParameter.pVertexShader = compileShaderCode(pGraphicsFrame, imguiVertexShader, getStringLength(imguiVertexShader), "main", nullptr, "ImGui VertexShader", shader_type_t::vertex_shader);
        pipelineParameter.pPixelShader = compileShaderCode(pGraphicsFrame, imguiPixelShader, getStringLength(imguiPixelShader), "main", nullptr, "ImGui PixelShader", shader_type_t::pixel_shader);
        pipelineParameter.topology = topology_t::triangle_list;
        pipelineParameter.pVertexFormat = pSampleContext->pSampleRenderQuadVertexFormat;

        texture_sampler_parameter_t samplerParameters = {};
        samplerParameters.addressModeU = texture_sampler_address_mode_type_t::mirror;
        samplerParameters.addressModeV = texture_sampler_address_mode_type_t::mirror;
        samplerParameters.addressModeW = texture_sampler_address_mode_type_t::mirror;
        samplerParameters.magnificationFilter = texture_sampler_filter_type_t::point;
        samplerParameters.minifactionFilter = texture_sampler_filter_type_t::point;
        pSampleContext->pSampler = createTextureSampler(pGraphicsFrame, &samplerParameters);
        pSampleContext->pSampleRenderGraphicsPipeline = createGraphicsPipeline(pGraphicsFrame, &pipelineParameter);
        pSampleContext->initialized = true;

        releaseShaderBinary(pGraphicsFrame, pipelineParameter.pVertexShader);
        releaseShaderBinary(pGraphicsFrame, pipelineParameter.pPixelShader);
    }

    if(pSampleContext->newWindowHeight != pSampleContext->windowHeight || pSampleContext->newWindowWidth != pSampleContext->windowWidth || pSampleContext->frameIndex == 1u)
    {
        pSampleContext->windowWidth = pSampleContext->newWindowWidth;
        pSampleContext->windowHeight = pSampleContext->newWindowHeight;
        
        ImGuiStyle* pGuiStyle = &ImGui::GetStyle();
        const float guiFontSize = ImGui::GetFontSize();
        const float borderSize = pGuiStyle->WindowBorderSize + pGuiStyle->FrameBorderSize + pGuiStyle->FramePadding.x;
        const float tabItemX = borderSize;
        const float tabItemWidth = pSampleContext->windowWidth - borderSize * 2.0f;

        //window title
        float tabItemY = guiFontSize + pGuiStyle->WindowBorderSize + pGuiStyle->FrameBorderSize + pGuiStyle->FramePadding.y * 2.0f;
        //window menu bar
        tabItemY += guiFontSize + pGuiStyle->ItemSpacing.y + pGuiStyle->FrameBorderSize + pGuiStyle->FramePadding.y * 2.0f;
        //tab header
        tabItemY += guiFontSize + pGuiStyle->ItemSpacing.y + pGuiStyle->FrameBorderSize + pGuiStyle->FramePadding.y * 2.0f;
        //tab content
        tabItemY += pGuiStyle->FramePadding.y + pGuiStyle->FrameBorderSize;

        const float tabItemHeight = pSampleContext->windowHeight - (tabItemY + borderSize);
        resizeSampleRenderTarget(pSampleContext, pGraphicsFrame, tabItemWidth, tabItemHeight);
        createSampleRenderQuad(pSampleContext, pGraphicsFrame, tabItemX, tabItemY, tabItemWidth, tabItemHeight, (float)pSampleContext->windowWidth, (float)pSampleContext->windowHeight);
    }

    doSampleFrame(pSampleContext, pGraphicsFrame);
    finishFrame(pSampleContext->pRenderContext, pGraphicsFrame);
    QueryPerformanceCounter(&endTime);

    handleMainWindowTitleBarLogic(pSampleContext);

    const LONGLONG frameTimeDelta = endTime.QuadPart - startTime.QuadPart;
    pSampleContext->deltaTimeInMs = ((float)frameTimeDelta / (float)pSampleContext->performanceFrequency.QuadPart) * 1000.f;
    ++pSampleContext->frameIndex;

    pSampleContext->totalFrameTimeInMs += pSampleContext->deltaTimeInMs;
}

void handleWindowResize(sample_context_t* pSampleContext, const uint32_t newWidth, const uint32_t newHeight)
{
    pSampleContext->newWindowWidth = newWidth;
    pSampleContext->newWindowHeight = newHeight;

    resizeBackBuffer(pSampleContext->pRenderContext, newWidth, newHeight);
    doSampleGuiFrame(pSampleContext);
}

void windowDoubleClick(HWND hwnd, LPARAM lparam)
{
    sample_context_t* pSampleContext = (sample_context_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pSampleContext == nullptr)
    {
        return;
    }

    POINT mousePos = {};
    mousePos.x = LOWORD(lparam);
    mousePos.y = HIWORD(lparam);
    ScreenToClient(hwnd, &mousePos);

    const int titleBarHeight = (int)(ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f);
    const bool doubleClickInTitleBar = mousePos.y < titleBarHeight;

    if( doubleClickInTitleBar )
    {
        pSampleContext->imguiState.mainWindowMaximized = true;
    }
}

void windowResizing(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    sample_context_t* pSampleContext = (sample_context_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pSampleContext == nullptr)
    {
        return;
    }

    const LPRECT pWindowSize = (LPRECT)lparam;
    const uint32_t newWidth = pWindowSize->right - pWindowSize->left;
    const uint32_t newHeight = pWindowSize->bottom - pWindowSize->top;
    handleWindowResize(pSampleContext, newWidth, newHeight);
}

void windowResized(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    sample_context_t* pSampleContext = (sample_context_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pSampleContext == nullptr)
    {
        return;
    }

    const uint32_t newWidth = LOWORD(lparam);
    const uint32_t newHeight = HIWORD(lparam);
    handleWindowResize(pSampleContext, newWidth, newHeight);
}

void windowChanged(HWND hwnd, LPARAM lparam)
{
    const WINDOWPOS* pWindowPos = (WINDOWPOS*)lparam;

    window_parameter_t windowParameter = {};
    windowParameter.x = pWindowPos->x;
    windowParameter.y = pWindowPos->y;
    windowParameter.height = pWindowPos->cy;
    windowParameter.width = pWindowPos->cx;
    writeWindowParametersToRegistry(&windowParameter);
}

int windowTitleBarMouseCheck(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    POINT pt = { LOWORD(lparam), HIWORD(lparam) };
    ScreenToClient(hwnd, &pt);

    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);
    
    const int windowWidth = clientRect.right - clientRect.left;
    const int windowHeight = clientRect.bottom - clientRect.top;
    const int borderSize = (int)ImGui::GetStyle().WindowBorderSize + 2;
    if(pt.y <= borderSize)
    {
        if(pt.x <= borderSize)
        {
            return HTTOPLEFT;
        }
        else if(pt.x >= windowWidth - borderSize)
        {
            return HTTOPRIGHT;
        }
        return HTTOP;
    }
    if(pt.y >= windowHeight - borderSize)
    {
        if(pt.x <= borderSize)
        {
            return HTBOTTOMLEFT;
        }
        else if(pt.x >= windowWidth - borderSize)
        {
            return HTBOTTOMRIGHT;
        }
        return HTBOTTOM;
    }
    if(pt.x <= borderSize)
    {
        return HTLEFT;
    }
    if(pt.x >= windowWidth - borderSize)
    {
        return HTRIGHT;
    }

    const int buttonWidth = (int)(ImGui::GetStyle().ItemInnerSpacing.x + ImGui::GetFontSize());
    const int titleBarButtonStartLeft = windowWidth - (int)(ImGui::GetStyle().WindowBorderSize + ImGui::GetStyle().FramePadding.x + 3.0f * buttonWidth);
    const int titleBarHeight = (int)(ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f);
    if (pt.y < titleBarHeight && pt.x < titleBarButtonStartLeft)
    {
        return HTCAPTION;
    }
    return HTCLIENT;
}

LRESULT CALLBACK D3D12TestAppWindowProc(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(p_HWND, p_Message, p_wParam, p_lParam))
        return true;

	bool messageHandled = false;

	switch (p_Message)
	{
    case WM_NCHITTEST:
        return windowTitleBarMouseCheck(p_HWND, p_wParam, p_lParam);
	case WM_CLOSE:
        DestroyWindow(p_HWND);
		messageHandled = true;
		break;

    case WM_DESTROY:
		PostQuitMessage(0);
		messageHandled = true;
        break;

    case WM_NCLBUTTONDBLCLK:
        windowDoubleClick(p_HWND, p_lParam);
        messageHandled = true;
        break;
    
    case WM_SIZING:
        windowResizing(p_HWND, p_wParam, p_lParam);
        break;

    case WM_SIZE:
        windowResized(p_HWND, p_Message, p_wParam, p_lParam);
        break;
    
    case WM_WINDOWPOSCHANGED:
        windowChanged(p_HWND, p_lParam);
        break;
	}

	if (messageHandled == false)
	{
		return DefWindowProc(p_HWND, p_Message, p_wParam, p_lParam);
	}

	return 0;
}

HWND setupWindow(HINSTANCE hInstance, int x, int y, int width, int height, const char* pWindowTitle)
{
	WNDCLASS wndClass = {0};
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW | CS_DBLCLKS;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = "D3D12RenderWindow";
	wndClass.lpfnWndProc = D3D12TestAppWindowProc;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowA("D3D12RenderWindow", pWindowTitle, WS_POPUP, x, y, width, height, 0, 0, hInstance, 0);

	if (hwnd == INVALID_HANDLE_VALUE)
		MessageBox(0, "Error creating Window.\n", "Error!", 0);
	else
		ShowWindow(hwnd, SW_SHOW);
	return hwnd;
}

void ImGuiAllocateDescriptor(ImGui_ImplDX12_InitInfo* pInitInfo, D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pOutGpuHandle)
{
    descriptor_heap_t* pDescriptorHeap = (descriptor_heap_t*)pInitInfo->UserData;
    descriptor_handle_t descriptorHandle = {};
    if(!allocateDescriptor(&descriptorHandle, pDescriptorHeap))
    {
        return;
    }

    *pOutCpuHandle = descriptorHandle.cpuDescriptorHandle;
    *pOutGpuHandle = descriptorHandle.gpuDescriptorHandle;
}

void ImGuiFreeDescriptor(ImGui_ImplDX12_InitInfo* pInitInfo, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{

}

bool initializeImGui(HWND pWindowHandle, D3D12DeviceType* pDevice, ID3D12CommandQueue* pCommandQueue, descriptor_heap_t* pDescriptorHeap, const uint32_t frameBufferCount)
{
    com_auto_release_t<ID3D12Device> pD3D12Device = nullptr;
    if(COM_CALL(pDevice->QueryInterface(IID_PPV_ARGS(&pD3D12Device))) != S_OK)
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; 

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = pD3D12Device.pPointer;
    init_info.CommandQueue = pCommandQueue;
    init_info.NumFramesInFlight = (int)frameBufferCount;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // Or your render target format.
    init_info.UserData = (void*)pDescriptorHeap;
    init_info.SrvDescriptorHeap = pDescriptorHeap->pDescriptorHeap;
    init_info.SrvDescriptorAllocFn = ImGuiAllocateDescriptor;
    init_info.SrvDescriptorFreeFn = ImGuiFreeDescriptor;
    
    ImGui_ImplWin32_Init(pWindowHandle);
    ImGui_ImplDX12_Init(&init_info);
    return true;
}

void sampleMainLoop(HWND pWindowHandle, render_context_t* pRenderContext)
{
    float frameTimeDeltaInMs = 0.0f;
    uint32_t frameIndex = 0;
    LARGE_INTEGER performanceFrequency;;
	QueryPerformanceFrequency(&performanceFrequency);

    sample_context_t sampleContext = {};
    sampleContext.initialized = false;
    sampleContext.pRenderContext = pRenderContext;
    sampleContext.pWindowHandle = pWindowHandle;
    sampleContext.imguiState.mainWindowOpen = true;
    sampleContext.performanceFrequency = performanceFrequency;
    sampleContext.activeSampleIndex = ~0;
    
    createDefaultMemoryAllocator(&sampleContext.allocator);

    RECT clientRect = {};
    GetClientRect(pWindowHandle, &clientRect);
    sampleContext.newWindowWidth = clientRect.right - clientRect.left;
    sampleContext.newWindowHeight = clientRect.bottom - clientRect.top;

    SetWindowLongPtrA(pWindowHandle, GWLP_USERDATA, (LONG_PTR)&sampleContext);
    while(true)
    {
        if(!pumpWin32Messages())
        {
            break;
        }

        doSampleGuiFrame(&sampleContext);
    }
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    window_parameter_t sampleWindowParameter = {};
    sampleWindowParameter.x = 0;
    sampleWindowParameter.y = 0;
    sampleWindowParameter.width = 1024;
    sampleWindowParameter.height = 768;

    if(readWindowParametersFromRegistry(&sampleWindowParameter))
    {
        if(!isValidWindowParameters(&sampleWindowParameter))
        {
            sampleWindowParameter.x = 0;
            sampleWindowParameter.y = 0;
            sampleWindowParameter.width = 1024;
            sampleWindowParameter.height = 768;
        }
    }

    HWND pWindowHandle = setupWindow(hInstance, sampleWindowParameter.x, sampleWindowParameter.y, sampleWindowParameter.width, sampleWindowParameter.height, "K15 D3D12 Renderer Samples");
    if(pWindowHandle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    RECT clientRect = {};
    GetClientRect(pWindowHandle, &clientRect);

    const uint32_t clientRectWidth  = (uint32_t)(clientRect.right - clientRect.left);
    const uint32_t clientRectHeight = (uint32_t)(clientRect.bottom - clientRect.top);
    render_context_t renderContext = {};

    const uint32_t frameBufferCount = 3u;
    const bool useDebugLayer = true;
    const render_context_parameters_t renderContextParameters = createDefaultRenderContextParameters(pWindowHandle, frameBufferCount, clientRectWidth, clientRectHeight, useDebugLayer);
    if(!createRenderContext(&renderContext, &renderContextParameters))
    {
        return -1;
    }

    pImGuiDescriptorHeap = (descriptor_heap_t*)malloc(sizeof(descriptor_heap_t));
    if(pImGuiDescriptorHeap == nullptr)
    {
        return -1;
    }

    if(!createDescriptorHeap(pImGuiDescriptorHeap, &renderContext.defaultAllocator, renderContext.pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 32))
    {
        return -1;
    }

    if(!initializeImGui(pWindowHandle, renderContext.pDevice, renderContext.ppCommandQueues[(int)gpu_pass_type_t::render], pImGuiDescriptorHeap, frameBufferCount))
    {
        return -1;
    }

    sampleMainLoop(pWindowHandle, &renderContext);
    return 0;
}