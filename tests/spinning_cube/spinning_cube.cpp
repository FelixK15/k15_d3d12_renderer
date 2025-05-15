
const char spinningCubePixelShader[] = R"(
SamplerState mySampler : register(s0, space1);
Texture2D texture : register(t0, space2);

struct PixelInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

float4 main(PixelInput input) : SV_Target
{
    return texture.Sample(mySampler, input.uv);
}   
)";

const char spinningCubeVertexShader[] = R"(
cbuffer SpinningCubeData : register(b0, space0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjMatrix2;
    float4x4 modelMatrix;
};

struct VertexInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

VertexOutput main(VertexInput vertexInput)
{
    float4x4 viewProjMatrix = mul(viewMatrix, projMatrix);
    float4x4 modelViewProjMatrix = mul(modelMatrix, viewProjMatrix);

    VertexOutput output;
    output.pos = mul(float4(vertexInput.pos, 1.0f), modelViewProjMatrix);
    output.uv = vertexInput.uv;
    output.normal = vertexInput.normal;
    return output;
}
)";

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
    gpu_texture_t* pTexture;
    gpu_texture_view_t* pTextureView;
    texture_sampler_t* pSampler;

    spinning_cube_constant_buffer_data_t spinningCubeData;
    gpu_buffer_t* pSpinningCubeConstantBuffer;
};

void doSpinningCubeSample(sample_frame_parameter_t* pFrameParameter)
{
    spinning_cube_test_data_t* pTestData = (spinning_cube_test_data_t*)pFrameParameter->pUserData;

    //rotate around y axis
    matrix4x4f_t* pModelMatrix = &pTestData->spinningCubeData.modelMatrix;
    pModelMatrix->m00 = cosf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m02 = sinf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m20 = -sinf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m22 = cosf(pFrameParameter->totalFrameTimeInMs / 1000.0f);

    gpu_buffer_t* pUploadBuffer = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(spinning_cube_constant_buffer_data_t), &pTestData->spinningCubeData, gpu_buffer_usage_flag_t::constant_buffer, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Spinning Cube Const Data");
    copyGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffer, pUploadBuffer);
    releaseGpuBuffer(pFrameParameter->pGraphicsFrame, pUploadBuffer);

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Draw Cube", pFrameParameter->pRenderTarget);
    clearColorRenderTarget(pRenderPass, pFrameParameter->pRenderTarget, 1.0f, 1.0f, 1.0f, 1.0f);
    bindGraphicsPipeline(pRenderPass, pTestData->pMaterial->pGraphicsPipeline);
    bindConstantBuffer(pRenderPass, pTestData->pSpinningCubeConstantBuffer, 0u, 0u);
    bindTextureSampler(pRenderPass, pTestData->pSampler, 0u, 1u);
    bindTexture(pRenderPass, pTestData->pTexture, 0u, 2u);
    drawIndexedMesh(pRenderPass, pTestData->pMesh, pTestData->pMaterial);

    endRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);   
    executeRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);
}

bool initSpinningCubeSample(sample_frame_parameter_t* pFrameParameter)
{
    spinning_cube_test_data_t* pTestData = (spinning_cube_test_data_t*)allocateFromAllocator(pFrameParameter->pAllocator, sizeof(spinning_cube_test_data_t), alloc_flags_t::clear_memory);
    if(pTestData == nullptr)
    {
        return false;
    }

    indexed_mesh_t* pMesh = createUnitCubeIndexedMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator);
    if(pMesh == nullptr)
    {
        freeFromAllocator(pFrameParameter->pAllocator, pTestData);
        return false;
    }

    material_t* pMaterial = createMaterial(pFrameParameter->pGraphicsFrame, "Spinning Cube Material", pFrameParameter->pAllocator, pMesh->pVertexFormat, spinningCubeVertexShader, spinningCubePixelShader);
    if(pMaterial == nullptr)
    {
        freeFromAllocator(pFrameParameter->pAllocator, pTestData);
        destroyIndexedMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pMesh);
        return false;
    }

    int textureWidth = 0, textureHeight = 0, channelsInTexture = 0;
    const stbi_uc* pImageData = stbi_load("smiley.png", &textureWidth, &textureHeight, &channelsInTexture, 4);

    setIdentityMatrix(&pTestData->spinningCubeData.modelMatrix);

    matrix4x4f_t projectionMatrix = {};
    matrix4x4f_t viewMatrix = {};

    setIdentityMatrix(&viewMatrix);
    createProjectionMatrix(&projectionMatrix, pFrameParameter->windowWidth, pFrameParameter->windowHeight, 0.1f, 10.0f, 80.0f);

    viewMatrix.m23 = 2.0f;

    texture_sampler_parameter_t samplerParameter = {};
    samplerParameter.mipmapFilter = texture_sampler_filter_type_t::linear;
    samplerParameter.minifactionFilter = texture_sampler_filter_type_t::linear;
    samplerParameter.magnificationFilter = texture_sampler_filter_type_t::linear;
    samplerParameter.addressModeU = texture_sampler_address_mode_type_t::clamp;
    samplerParameter.addressModeV = texture_sampler_address_mode_type_t::clamp;
    samplerParameter.addressModeW = texture_sampler_address_mode_type_t::clamp;
    pTestData->pSampler = createTextureSampler(pFrameParameter->pGraphicsFrame, &samplerParameter);

    pTestData->spinningCubeData.viewMatrix = viewMatrix;
    pTestData->spinningCubeData.projMatrix = projectionMatrix;
    pTestData->spinningCubeData.viewProjMatrix = mulMatrices(&viewMatrix, &projectionMatrix);
    pTestData->pSpinningCubeConstantBuffer = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(spinning_cube_constant_buffer_data_t), nullptr, gpu_buffer_usage_flag_t::constant_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess);
    pTestData->pTexture = createGpuTexture(pFrameParameter->pGraphicsFrame, createUint3(textureWidth, textureHeight, 1), 1u, pImageData, gpu_texture_usage_flag_t::shader_resource_view, gpu_texture_format_t::R8G8B8A8, gpu_texture_format_type_t::normalized_unsigned_int);

    pTestData->pMesh = pMesh;
    pTestData->pMaterial = pMaterial;

    pFrameParameter->pUserData = pTestData;
    return true;
}

void shutdownSpinningCubeSample(sample_frame_parameter_t* pFrameParameter)
{
    spinning_cube_test_data_t* pTestData = (spinning_cube_test_data_t*)pFrameParameter->pUserData;
    releaseGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffer);
    releaseGpuTexture(pFrameParameter->pGraphicsFrame, pTestData->pTexture);
    releaseTextureSampler(pFrameParameter->pGraphicsFrame, pTestData->pSampler);

    destroyMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMaterial);
    destroyIndexedMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMesh);
    freeFromAllocator(pFrameParameter->pAllocator, pTestData);
}