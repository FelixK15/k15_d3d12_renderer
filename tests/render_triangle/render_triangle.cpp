
#include "../../k15_d3d12_renderer.hpp"
#include "../test_base.hpp"

struct render_triangle_test_data_t
{
    mesh_t* pMesh;
    material_t* pMaterial;
};

mesh_t* createMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, const float* pVertices, const uint32_t vertexCount, vertex_format_t* pVertexFormat)
{
    const uint32_t vertexBufferSizeInBytes = vertexCount * calculateVertexStrideSizeInBytes(pVertexFormat);
    upload_buffer_t* pVertexUploadBuffer = createUploadBuffer(pGraphicsFrame, vertexBufferSizeInBytes);
    memcpy(pVertexUploadBuffer->pData, pVertices, vertexBufferSizeInBytes);

    vertex_buffer_t* pMeshVertexBuffer = createVertexBuffer(pGraphicsFrame, pVertexUploadBuffer);

    mesh_t* pMesh = (mesh_t*)allocateFromAllocator(pMemoryAllocator, sizeof(mesh_t), alloc_flag_clear_memory);
    pMesh->vertexCount = vertexCount;
    pMesh->vertexOffset = 0u;
    pMesh->pVertexFormat = pVertexFormat;
    pMesh->pVertexBuffer = pMeshVertexBuffer;

    releaseUploadBuffer(pGraphicsFrame, pVertexUploadBuffer);

    return pMesh;
}

material_t* createMaterial(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, vertex_format_t* pVertexFormat, const shader_compilation_parameters_t* pVertexShaderParameters, const shader_compilation_parameters_t* pPixelShaderParameters)
{
    graphics_pipeline_state_parameters_t pipelineStateParameters = {};
    pipelineStateParameters.pVertexShader   = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pVertexShaderParameters);
    pipelineStateParameters.pPixelShader    = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pPixelShaderParameters);
    pipelineStateParameters.pVertexFormat   = pVertexFormat;
    pipelineStateParameters.pName           = "Test";

    graphics_pipeline_state_t* pDefaultPipelineStateObject = createGraphicsPipelineState(pGraphicsFrame, &pipelineStateParameters);
    material_t* pMaterial = (material_t*)allocateFromAllocator(pMemoryAllocator, sizeof(material_t), alloc_flag_clear_memory);
    pMaterial->pGraphicsPipelineState = pDefaultPipelineStateObject;
    return pMaterial;
}

void destroyMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, mesh_t* pMesh)
{
    releaseVertexBuffer(pGraphicsFrame, pMesh->pVertexBuffer);
    releaseVertexFormat(pGraphicsFrame, pMesh->pVertexFormat);
    freeFromAllocator(pMemoryAllocator, pMesh);
}

void destroyMaterial(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, material_t* pMaterial)
{
    releaseGraphicsPipeline(pGraphicsFrame, pMaterial->pGraphicsPipelineState);
    freeFromAllocator(pMemoryAllocator, pMaterial);
}

mesh_t* createSingleTriangleMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator)
{
    const float triangleVertices[] = {
        0.0f, 0.0f, 0.5f,
        1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, 1.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 1.0f
    };

    vertex_attribute_entry_t pVertexAttributes[] = {
        {vertex_attribute_t::position, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 3u},
        {vertex_attribute_t::color, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 12u, 4u}
    };

    vertex_format_t* pVertexFormat = createVertexFormat(pGraphicsFrame, pVertexAttributes, 2u);

    return createMesh(pGraphicsFrame, pMemoryAllocator, triangleVertices, 3u, pVertexFormat);
}

void drawMesh(mesh_t* pMesh, material_t* pMaterial, render_pass_t* pRenderPass)
{
	bindGraphicsPipelineState(pRenderPass, pMaterial->pGraphicsPipelineState);
	bindVertexBuffer(pRenderPass, pMesh->pVertexBuffer, pMesh->pVertexFormat, 0u);
	draw(pRenderPass, pMesh->vertexOffset, pMesh->vertexCount);
}

void doFrame(test_context_frame_parameter_t* pFrameParameter)
{
    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)pFrameParameter->pUserData;

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Draw Triangle", pFrameParameter->pGraphicsFrame->pBackBuffer);
    clearColorRenderTarget(pRenderPass, pFrameParameter->pGraphicsFrame->pBackBuffer, 0.0f, 0.0f, 0.0f, 1.0f);
    drawMesh(pTestData->pMesh, pTestData->pMaterial, pRenderPass);

    endRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);   
    executeRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);
}

bool initTest(test_context_frame_parameter_t* pFrameParameter)
{
    shader_compilation_parameters_t vs_para = {};
    vs_para.pEntryPoint = "main";
    vs_para.pFilePath = "vertex_shader.hlsl";
    vs_para.pShaderProfile = "vs_6_0";

    shader_compilation_parameters_t ps_para = vs_para;
    ps_para.pFilePath = "pixel_shader.hlsl";
    ps_para.pShaderProfile = "ps_6_0";

    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)allocateFromAllocator(pFrameParameter->pAllocator, (sizeof(render_triangle_test_data_t), alloc_flag_clear_memory));
    if(pTestData == nullptr)
    {
        return false;
    }

    mesh_t* pMesh = createSingleTriangleMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator);
    if(pMesh == nullptr)
    {
        free(pTestData);
        return false;
    }

    material_t* pMaterial = createMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pMesh->pVertexFormat, &vs_para, &ps_para);
    if(pMaterial == nullptr)
    {
        free(pTestData);
        destroyMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pMesh);
        return false;
    }
    pTestData->pMesh = pMesh;
    pTestData->pMaterial = pMaterial;

    pFrameParameter->pUserData = pTestData;
    return true;
}

void shutdownTest(test_context_frame_parameter_t* pFrameParameter)
{
    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)pFrameParameter->pUserData;
    destroyMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMaterial);
    destroyMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMesh);
    freeFromAllocator(pFrameParameter->pAllocator, pTestData);
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
    test_context_parameters_t parameters = {};
    parameters.useDebugLayer = true;
    parameters.pFrameCallback = doFrame;
    parameters.pInitCallback = initTest;
    parameters.pShutdownCallback = shutdownTest;

    result_t<test_context_t> testContextResult = initTestEnvironmentAndWindow(hInstance, 1024, 768, "[DX12] render triangle", &parameters);
    if(!isResultSuccessful(testContextResult))
    {
        return -1;
    }

    return startTest(&testContextResult.value);
}