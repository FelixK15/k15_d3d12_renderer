
const char renderTriangleVertexShader[] = R"(
struct VertexInput
{
    float4 pos : POSITION;
    float3 color : COLOR;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput output;
    output.pos = vertexInput.pos;
    output.color = float4(vertexInput.color, 1.0f);
    return output;
})";

const char renderTrianglePixelShader[] = R"(
struct PixelInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PixelInput input) : SV_Target
{
    return input.color;
})";

struct render_triangle_test_data_t
{
    mesh_t* pMesh;
    material_t* pMaterial;
};

void doRenderTriangleSample(sample_frame_parameter_t* pFrameParameter)
{
    render_triangle_test_data_t* pTestData = (render_triangle_test_data_t*)pFrameParameter->pUserData;

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Draw Triangle", pFrameParameter->pRenderTarget);
    clearColorRenderTarget(pRenderPass, pFrameParameter->pRenderTarget, 0.0f, 0.0f, 0.0f, 1.0f);
    drawMesh(pRenderPass, pTestData->pMesh, pTestData->pMaterial);

    endRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);   
    executeRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);
}

bool initRenderTriangleSample(sample_frame_parameter_t* pFrameParameter)
{
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

    material_t* pMaterial = createMaterial(pFrameParameter->pGraphicsFrame, "Render Triangle Material", pFrameParameter->pAllocator, pMesh->pVertexFormat, renderTriangleVertexShader, renderTrianglePixelShader);
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
    destroyMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMaterial);
    destroyMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMesh);
    freeFromAllocator(pFrameParameter->pAllocator, pTestData);
}