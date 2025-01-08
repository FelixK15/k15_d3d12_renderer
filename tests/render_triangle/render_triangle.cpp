
#include "../../k15_d3d12_renderer.hpp"
#include "../test_base.hpp"

mesh_t* createMesh(graphics_frame_t* pGraphicsFrame, const float* pVertices, const uint32_t vertexCount, vertex_format_t* pVertexFormat)
{
    const uint32_t vertexBufferSizeInBytes = vertexCount * calculateVertexStrideSizeInBytes(pVertexFormat);
    upload_buffer_t* pVertexUploadBuffer = createUploadBuffer(pGraphicsFrame, vertexBufferSizeInBytes);
    memcpy(pVertexUploadBuffer->pData, pVertices, vertexBufferSizeInBytes);

    vertex_buffer_t* pMeshVertexBuffer = createVertexBuffer(pGraphicsFrame, pVertexUploadBuffer);

    mesh_t* pMesh = (mesh_t*)allocateFromDefaultAllocator(nullptr, sizeof(mesh_t), defaultAllocationAlignment);
    pMesh->vertexCount = vertexCount;
    pMesh->vertexOffset = 0u;
    pMesh->pVertexFormat = pVertexFormat;
    pMesh->pVertexBuffer = pMeshVertexBuffer;

    return pMesh;
}

material_t* createMaterial(graphics_frame_t* pGraphicsFrame, vertex_format_t* pVertexFormat, const shader_compilation_parameters_t* pVertexShaderParameters, const shader_compilation_parameters_t* pPixelShaderParameters)
{
    graphics_pipeline_state_parameters_t pipelineStateParameters = {};
    pipelineStateParameters.pVertexShader   = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pVertexShaderParameters);
    pipelineStateParameters.pPixelShader    = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pPixelShaderParameters);
    pipelineStateParameters.pVertexFormat   = pVertexFormat;
    pipelineStateParameters.pName           = "Test";

    graphics_pipeline_state_t* pDefaultPipelineStateObject = createGraphicsPipelineState(pGraphicsFrame, &pipelineStateParameters);
    material_t* pMaterial = (material_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, sizeof(material_t));
    pMaterial->pGraphicsPipelineState = pDefaultPipelineStateObject;
    return pMaterial;
}

mesh_t* createSingleTriangleMesh(graphics_frame_t* pGraphicsFrame)
{
    const float triangleVertices[] = {
        0.0f, 0.0f, 0.5f,
        1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, 1.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 1.0f
    };

    //render_pass_t* pDrawRenderPass = startRenderPass(pGraphicsFrame, "Draw Triangle");
    vertex_attribute_entry_t pVertexAttributes[] = {
        {vertex_attribute_t::position, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 3u},
        {vertex_attribute_t::color, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 12u, 4u}
    };

    vertex_format_t* pVertexFormat = createVertexFormat(pGraphicsFrame, pVertexAttributes, 2u);

    return createMesh(pGraphicsFrame, triangleVertices, 3u, pVertexFormat);
}

void renderFrame(HWND hwnd, graphics_frame_t* pGraphicsFrame)
{
    POINT cursorPos;
    RECT clientRect;
    GetCursorPos(&cursorPos);
    GetClientRect(hwnd, &clientRect);

    shader_compilation_parameters_t vs_para = {};
    vs_para.pEntryPoint = "main";
    vs_para.pFilePath = "vertex_shader.hlsl";
    vs_para.pShaderProfile = "vs_6_0";

    shader_compilation_parameters_t ps_para = vs_para;
    ps_para.pFilePath = "pixel_shader.hlsl";
    ps_para.pShaderProfile = "ps_6_0";

    static mesh_t* pMesh = createSingleTriangleMesh(pGraphicsFrame);
    static material_t* pMaterial = createMaterial(pGraphicsFrame, pMesh->pVertexFormat, &vs_para, &ps_para);

    const float r = (float)cursorPos.x / (float)(clientRect.right - clientRect.left);
    const float g = (float)cursorPos.y / (float)(clientRect.bottom - clientRect.top);

    const FLOAT backBufferRGBA[4] = {r, g, 0.2f, 1.0f};

    render_pass_t* pRenderPass = startRenderPass(pGraphicsFrame, "Draw Triangle", pGraphicsFrame->pBackBuffer);
    clearColorRenderTarget(pRenderPass, pGraphicsFrame->pBackBuffer, r, g, 0.2f, 1.0f);
    drawMesh(pMesh, pMaterial, pRenderPass);
    endRenderPass(pGraphicsFrame, pRenderPass);   

    executeRenderPass(pGraphicsFrame, pRenderPass);
}

void doFrame(const test_context_frame_parameter_t* pFrameParameter)
{
    static bool meshCreated = false;
    graphics_frame_t* pFrame = beginNextFrame(pFrameParameter->pRenderContext);
    
    #if 0
    if(!meshCreated && createMesh(pFrame))
    {
        meshCreated = true;
    }
    #endif

    renderFrame(pFrameParameter->pWindowHandle, pFrame);
    finishFrame(pFrameParameter->pRenderContext, pFrame);
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
    test_context_parameters_t parameters = {};
    parameters.useDebugLayer = true;
    parameters.pFrameCallback = doFrame;

    result_t<test_context_t> testContextResult = initTestEnvironmentAndWindow(hInstance, 1024, 768, "[DX12] render triangle", &parameters);
    if(!isResultSuccessful(testContextResult))
    {
        return -1;
    }

    return startTest(&testContextResult.value);
}