
#include "../../k15_d3d12_renderer.hpp"
#include "../test_base.hpp"

struct render_triangle_test_data_t
{
    mesh_t* pMesh;
    material_t* pMaterial;
};

void doFrame(test_context_frame_parameter_t* pFrameParameter)
{
    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)pFrameParameter->pUserData;

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Draw Triangle", pFrameParameter->pGraphicsFrame->pBackBuffer);
    clearColorRenderTarget(pRenderPass, pFrameParameter->pGraphicsFrame->pBackBuffer, 0.0f, 0.0f, 0.0f, 1.0f);
    drawMesh(pRenderPass, pTestData->pMesh, pTestData->pMaterial);

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

    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)allocateFromAllocator(pFrameParameter->pAllocator, (sizeof(render_triangle_test_data_t), alloc_flags_t::clear_memory));
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

    result_t<test_context_t*> testContextResult = initTestEnvironmentAndWindow(hInstance, 1024, 768, "[DX12] render triangle", &parameters);
    if(!isResultSuccessful(testContextResult))
    {
        return -1;
    }

    return startTest(testContextResult.value);
}