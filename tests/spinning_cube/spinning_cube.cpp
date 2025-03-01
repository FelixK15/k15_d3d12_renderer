
#include "../../k15_d3d12_renderer.hpp"
#include "../test_base.hpp"

struct spinning_cube_constant_buffer_data_t
{
    matrix4x4f_t viewMatrix;
    matrix4x4f_t projMatrix;
    matrix4x4f_t viewProjMatrix;
    matrix4x4f_t modelMatrix;
};

struct spinning_cube_test_data_t
{
    indexed_mesh_t* pMesh;
    material_t* pMaterial;

    spinning_cube_constant_buffer_data_t spinningCubeData;
    gpu_buffer_t* pSpinningCubeConstantBuffer;
};

void doFrame(test_context_frame_parameter_t* pFrameParameter)
{
    spinning_cube_test_data_t* pTestData = (spinning_cube_test_data_t*)pFrameParameter->pUserData;

    //rotate around y axis
    matrix4x4f_t* pModelMatrix = &pTestData->spinningCubeData.modelMatrix;
    pModelMatrix->m00 = cosf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m02 = sinf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m20 = -sinf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m22 = cosf(pFrameParameter->totalFrameTimeInMs / 1000.0f);

    gpu_buffer_t* pUploadBuffer = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(spinning_cube_constant_buffer_data_t), &pTestData->spinningCubeData, gpu_buffer_usage_t::constant_buffer, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Spinning Cube Const Data");
    copyGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffer, pUploadBuffer);
    freeGpuBuffer(pFrameParameter->pGraphicsFrame, pUploadBuffer);

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Draw Cube", pFrameParameter->pGraphicsFrame->pBackBuffer);
    clearColorRenderTarget(pRenderPass, pFrameParameter->pGraphicsFrame->pBackBuffer, 0.0f, 0.0f, 0.0f, 1.0f);
    
    bindGraphicsPipeline(pRenderPass, pTestData->pMaterial->pGraphicsPipeline);
    bindConstantBuffer(pRenderPass, pTestData->pSpinningCubeConstantBuffer, 0u);
    drawIndexedMesh(pRenderPass, pTestData->pMesh, pTestData->pMaterial);

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

    spinning_cube_test_data_t* pTestData = (spinning_cube_test_data_t*)allocateFromAllocator(pFrameParameter->pAllocator, sizeof(spinning_cube_test_data_t), alloc_flags_t::clear_memory);
    if(pTestData == nullptr)
    {
        return false;
    }

    indexed_mesh_t* pMesh = createRedUnitCubeIndexedMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator);
    if(pMesh == nullptr)
    {
        freeFromAllocator(pFrameParameter->pAllocator, pTestData);
        return false;
    }

    material_t* pMaterial = createMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pMesh->pVertexFormat, &vs_para, &ps_para);
    if(pMaterial == nullptr)
    {
        freeFromAllocator(pFrameParameter->pAllocator, pTestData);
        destroyIndexedMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pMesh);
        return false;
    }

    setIdentityMatrix(&pTestData->spinningCubeData.modelMatrix);

    matrix4x4f_t projectionMatrix = {};
    matrix4x4f_t viewMatrix = {};

    setIdentityMatrix(&viewMatrix);
    createProjectionMatrix(&projectionMatrix, pFrameParameter->windowWidth, pFrameParameter->windowHeight, 0.1f, 10.0f, 80.0f);

    viewMatrix.m23 = 5.0f;

    pTestData->spinningCubeData.viewMatrix = viewMatrix;
    pTestData->spinningCubeData.projMatrix = projectionMatrix;
    pTestData->spinningCubeData.viewProjMatrix = mulMatrices(&viewMatrix, &projectionMatrix);
    pTestData->pSpinningCubeConstantBuffer = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(spinning_cube_constant_buffer_data_t), nullptr, gpu_buffer_usage_t::constant_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess);

    pTestData->pMesh = pMesh;
    pTestData->pMaterial = pMaterial;

    pFrameParameter->pUserData = pTestData;
    return true;
}

void shutdownTest(test_context_frame_parameter_t* pFrameParameter)
{
    spinning_cube_test_data_t* pTestData = (spinning_cube_test_data_t*)pFrameParameter->pUserData;
    freeGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffer);

    destroyMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMaterial);
    destroyIndexedMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMesh);
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

    result_t<test_context_t*> testContextResult = initTestEnvironmentAndWindow(hInstance, 1024, 768, "[DX12] spinning cube", &parameters);
    if(!isResultSuccessful(testContextResult))
    {
        return -1;
    }

    return startTest(testContextResult.value);
}