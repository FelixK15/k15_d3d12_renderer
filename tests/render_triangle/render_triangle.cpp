
struct render_triangle_test_data_t
{
    mesh_t* pMesh;
    material_t* pMaterial;
};

void doRenderTriangleSample(sample_frame_parameter_t* pFrameParameter, const int x, const int y, const int width, const int height)
{
    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)pFrameParameter->pUserData;

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Draw Triangle", pFrameParameter->pGraphicsFrame->pBackBuffer);
    setViewport(pRenderPass, x, y, width, height, 0.0f, 100.f);
    setScissor(pRenderPass, x, y, width, height);
    //clearColorRenderTarget(pRenderPass, pFrameParameter->pGraphicsFrame->pBackBuffer, 0.0f, 0.0f, 0.0f, 1.0f);
    drawMesh(pRenderPass, pTestData->pMesh, pTestData->pMaterial);

    endRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);   
    executeRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);
}

bool initRenderTriangleSample(sample_frame_parameter_t* pFrameParameter)
{
    shader_compilation_parameters_t vs_para = {};
    vs_para.pEntryPoint = "main";
    vs_para.pFilePath = "render_triangle/vertex_shader.hlsl";
    vs_para.pShaderProfile = "vs_6_0";

    shader_compilation_parameters_t ps_para = vs_para;
    ps_para.pFilePath = "render_triangle/pixel_shader.hlsl";
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

    material_t* pMaterial = createMaterial(pFrameParameter->pGraphicsFrame, "Render Triangle Material", pFrameParameter->pAllocator, pMesh->pVertexFormat, &vs_para, &ps_para);
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

void shutdownRenderTriangleSample(sample_frame_parameter_t* pFrameParameter)
{
    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)pFrameParameter->pUserData;
    //destroyMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMaterial);
    destroyMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMesh);
    freeFromAllocator(pFrameParameter->pAllocator, pTestData);
}