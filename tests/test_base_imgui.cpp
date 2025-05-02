#define NOMINMAX
#include "imgui\imgui.h"
#include "imgui\backends\imgui_impl_win32.h"
#include "imgui\backends\imgui_impl_dx12.h"

#include "..\k15_d3d12_renderer.hpp"

#if 0
#include "clear_backbuffer\clear_backbuffer_sample.hpp"
#include "render_triangle\render_triangle_sample.hpp"
#include "spinning_cube\spinning_cube_sample.hpp"
#include "compute_texture\compute_texture_sample.hpp"
#endif

descriptor_heap_t* pImGuiDescriptorHeap = nullptr;

struct sample_imgui_state_t
{
    bool mainWindowOpen;
    bool showSamplesMenu;
};

struct sample_frame_parameter_t
{
    HWND                    pWindowHandle;
	memory_allocator_t*	    pAllocator;
    render_context_t*       pRenderContext;
	graphics_frame_t* 	    pGraphicsFrame;
	render_target_t* 	    pRenderTarget;

    sample_imgui_state_t    imguiState;

	void* 				    pUserData;

	float 				    deltaTimeInMs;
	float 				    totalFrameTimeInMs;

	uint32_t 			    windowWidth;
	uint32_t 			    windowHeight;

	uint32_t 			    frameIndex;
};

void windowResized(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    sample_frame_parameter_t* pSampleFrameParameter = (sample_frame_parameter_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pSampleFrameParameter == nullptr)
        return;

    const uint32_t newWidth = LOWORD(lparam);
    const uint32_t newHeight = HIWORD(lparam);

	pSampleFrameParameter->windowHeight = newHeight;
	pSampleFrameParameter->windowWidth = newWidth;
    resizeBackBuffer(pSampleFrameParameter->pRenderContext, newWidth, newHeight);
}

int dragWindow(HWND hwnd, WPARAM wparam, LPARAM lparam)
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

    const int titleBarHeight = (int)(ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f);
    if (pt.y < titleBarHeight && pt.x < windowWidth - 30)
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
        return dragWindow(p_HWND, p_wParam, p_lParam);
	case WM_CLOSE:
        DestroyWindow(p_HWND);
		messageHandled = true;
		break;

    case WM_DESTROY:
		PostQuitMessage(0);
		messageHandled = true;
        break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		break;

	case WM_MOUSEMOVE:
		break;

	case WM_MOUSEWHEEL:
		break;
    
    case WM_SIZE:
        windowResized(p_HWND, p_Message, p_wParam, p_lParam);
        messageHandled = true;
        break;
	}

	if (messageHandled == false)
	{
		return DefWindowProc(p_HWND, p_Message, p_wParam, p_lParam);
	}

	return 0;
}

HWND setupWindow(HINSTANCE hInstance, int width, int height, const char* pWindowTitle)
{
	WNDCLASS wndClass = {0};
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = "D3D12RenderWindow";
	wndClass.lpfnWndProc = D3D12TestAppWindowProc;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowA("D3D12RenderWindow", pWindowTitle,
		WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
		width, height, 0, 0, hInstance, 0);

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

bool pumpWin32Message()
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
    pImGuiRenderPass->pGpuCommandBuffer->pCommandList->OMSetRenderTargets(1u, &pImGuiRenderPass->pipelineState.pRenderTarget->colorBufferHandle, FALSE, nullptr);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pImGuiRenderPass->pGpuCommandBuffer->pCommandList);
    endRenderPass(pGraphicsFrame, pImGuiRenderPass);
    executeRenderPass(pGraphicsFrame, pImGuiRenderPass);
}

void startImGuiFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void doGeneralSampleImGuiFrame(sample_frame_parameter_t* pSampleFrameParameter)
{
    ImVec2 windowSize = {};
    windowSize.x = (float)pSampleFrameParameter->windowWidth;
    windowSize.y = (float)pSampleFrameParameter->windowHeight;
    
    sample_imgui_state_t* pImguiState = &pSampleFrameParameter->imguiState;
    if(!ImGui::Begin("K15 D3D12 Rendering Samples", &pImguiState->mainWindowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        return;
    }
    
    ImGui::SetWindowSize(windowSize, ImGuiCond_Always);

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("Samples", &pImguiState->showSamplesMenu))
        {
            ImGui::MenuItem("Bla");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

bool doSampleFrame(HWND pWindowHandle, render_context_t* pRenderContext, graphics_frame_t* pGraphicsFrame, sample_frame_parameter_t* pSampleFrameParameter, float deltaTimeInMs, uint32_t frameIndex)
{
    sample_frame_parameter_t sampleFrameParameter = {};
    sampleFrameParameter.pWindowHandle  = pWindowHandle;
    sampleFrameParameter.pRenderContext = pRenderContext;
    sampleFrameParameter.pGraphicsFrame = pGraphicsFrame;
    sampleFrameParameter.deltaTimeInMs  = deltaTimeInMs;
    sampleFrameParameter.frameIndex     = frameIndex;

    startImGuiFrame();
    //ImGui::ShowDemoWindow();
    doGeneralSampleImGuiFrame(pSampleFrameParameter);
    renderImGui(pGraphicsFrame);

    return pSampleFrameParameter->imguiState.mainWindowOpen;
}

void sampleMainLoop(HWND pWindowHandle, render_context_t* pRenderContext)
{
    float frameTimeDeltaInMs = 0.0f;
    uint32_t frameIndex = 0;
    LARGE_INTEGER performanceFrequency, endTime, startTime;
	QueryPerformanceFrequency(&performanceFrequency);

    sample_frame_parameter_t frameParameter = {};
    frameParameter.pRenderContext = pRenderContext;
    frameParameter.pWindowHandle = pWindowHandle;
    frameParameter.imguiState.mainWindowOpen = true;
    
    RECT clientRect = {};
    GetClientRect(pWindowHandle, &clientRect);
    frameParameter.windowWidth = clientRect.right - clientRect.left;
    frameParameter.windowHeight = clientRect.bottom - clientRect.top;

    SetWindowLongPtrA(pWindowHandle, GWLP_USERDATA, (LONG_PTR)&frameParameter);
    while(true)
    {
        if(!pumpWin32Message())
        {
            break;
        }

        QueryPerformanceCounter(&startTime);
        graphics_frame_t* pGraphicsFrame = beginNextFrame(pRenderContext);
        if(!doSampleFrame(pWindowHandle, pRenderContext, pGraphicsFrame, &frameParameter, frameTimeDeltaInMs, frameIndex))
        {
            break;
        }
        finishFrame(pRenderContext, pGraphicsFrame);
        QueryPerformanceCounter(&endTime);

        const LONGLONG frameTimeDelta = endTime.QuadPart - startTime.QuadPart;
        frameTimeDeltaInMs = ((float)frameTimeDelta / (float)performanceFrequency.QuadPart) * 1000.f;
        ++frameIndex;
#if 0
        pFrameParameter->pGraphicsFrame = beginNextFrame(pTestContext->pRenderContext);
        if(firstFrame)
        {
            if(pTestContext->pInitCallback != nullptr)
            {
                pTestContext->pInitCallback(pFrameParameter);
            }

            firstFrame = false;
        }

        pTestContext->pFrameCallback(pFrameParameter);

        if(loopRunning == false && pTestContext->pShutdownCallback != nullptr)
        {
            pTestContext->pShutdownCallback(pFrameParameter);
        }
        finishFrame(pTestContext->pRenderContext, pFrameParameter->pGraphicsFrame);
#endif
    }
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HWND pWindowHandle = setupWindow(hInstance, 1024, 768, "K15 D3D12 Renderer Samples");
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

    if(!createDescriptorHeap(pImGuiDescriptorHeap, renderContext.pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 32))
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