const char computeTexturePixelShader[] = R"(
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

const char computeTextureVertexShader[] = R"(
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

const char computeTextureComputeShader[] = R"(
cbuffer TextureGenBuffer : register(b0, space0)
{
    uint textureWidth;
    uint color0;
    uint color1;
};

RWStructuredBuffer<uint> textureBuffer : register(u0, space1);

[numthreads(8,8,1)]
void main( uint3 groupId : SV_GroupID )
{
    uint textureBlockStartX = groupId.x * 8;
    uint textureBlockStartY = groupId.y * 8;
    uint textureBlockEndX = textureBlockStartX + 8;
    uint textureBlockEndY = textureBlockStartY + 8;

    uint color = color0;
    if( ( groupId.x & 1 ) == 0 && ( groupId.y & 1 ) == 0 )
    {
        color = color1;
    }

    for(uint y = textureBlockStartY; y < textureBlockEndY; ++y)
    {
        for(uint x = textureBlockStartX; x < textureBlockEndX; ++x)
        {
            textureBuffer[x + y * textureWidth] = color;
        }
    }
}
)";

struct compute_texture_data_t
{
    uint32_t textureWidth;
    uint32_t color0;
    uint32_t color1;
};

struct compute_texture_test_data_t
{
    uint32_t textureWidth;
    uint32_t textureHeight;

    float oneSecondAccumulation;
    bool pingPong;

    indexed_mesh_t* pMesh;
    material_t* pMaterial;
    gpu_texture_t* pTexture;
    texture_sampler_t* pSampler;

    spinning_cube_constant_buffer_data_t spinningCubeData;
    compute_texture_data_t computeTextureData;
    gpu_buffer_t* pSpinningCubeConstantBuffers[2];
    gpu_buffer_t* pComputeConstBuffers[2];
    gpu_buffer_t* pComputeTextureBuffer;
    shader_binary_t* pComputeShader;
    compute_pipeline_t* pComputePipeline;
};

void doComputeTextureSample(sample_frame_parameter_t* pFrameParameter)
{
    compute_texture_test_data_t* pTestData = (compute_texture_test_data_t*)pFrameParameter->pUserData;

    //rotate around y axis
    matrix4x4f_t* pModelMatrix = &pTestData->spinningCubeData.modelMatrix;
    pModelMatrix->m00 = cosf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m02 = sinf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m20 = -sinf(pFrameParameter->totalFrameTimeInMs / 1000.0f);
    pModelMatrix->m22 = cosf(pFrameParameter->totalFrameTimeInMs / 1000.0f);

    const uint32_t onlineBufferIndex = pFrameParameter->frameIndex & 1;
    const uint32_t offlineBufferIndex = ~onlineBufferIndex & 0x1;

    spinning_cube_constant_buffer_data_t* pSpinningCubeData = (spinning_cube_constant_buffer_data_t*)mapGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffers[offlineBufferIndex], 0u, 0u);    
    memcpy(pSpinningCubeData, &pTestData->spinningCubeData, sizeof(spinning_cube_constant_buffer_data_t));
    unmapGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffers[offlineBufferIndex]);

    if(pTestData->pingPong)
    {
        pTestData->oneSecondAccumulation += pFrameParameter->deltaTimeInMs / 1000.f;
        if(pTestData->oneSecondAccumulation >= 1.0f)
        {
            pTestData->pingPong = false;
            pTestData->oneSecondAccumulation = 1.0f;
        }
    }
    else
    {
        pTestData->oneSecondAccumulation -= pFrameParameter->deltaTimeInMs / 1000.f;
        if(pTestData->oneSecondAccumulation <= 0.0f)
        {
            pTestData->pingPong = true;
            pTestData->oneSecondAccumulation = 0.0f;
        }
    }

    compute_texture_data_t* pTextureData = (compute_texture_data_t*)mapGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pComputeConstBuffers[offlineBufferIndex], 0u, 0u);
    const uint8_t color0Component = (uint8_t)(pTestData->oneSecondAccumulation * 255.f);
    const uint8_t color1Component = (uint8_t)((1.0f - pTestData->oneSecondAccumulation) * 255.f);
    pTextureData->color0 = 0xFF000000 | color0Component;
    pTextureData->color1 = 0xFF000000 | (color1Component << 16u);
    unmapGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pComputeConstBuffers[offlineBufferIndex]);

    render_pass_t* pComputePass = startComputePass(pFrameParameter->pGraphicsFrame, "Generate Texture");
    bindComputePipeline(pComputePass, pTestData->pComputePipeline);
    bindStructuredBuffer(pComputePass, pTestData->pComputeTextureBuffer, 0, 1);
    bindConstantBuffer(pComputePass, pTestData->pComputeConstBuffers[onlineBufferIndex], 0, 0);
    dispatch(pComputePass, pTestData->textureWidth / 8, pTestData->textureHeight / 8, 1);
    copyGpuTextureFromBuffer(pComputePass, pTestData->pTexture, pTestData->pComputeTextureBuffer);
    endRenderPass(pFrameParameter->pGraphicsFrame, pComputePass);
    executeRenderPass(pFrameParameter->pGraphicsFrame, pComputePass);

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Draw Cube", pFrameParameter->pRenderTarget);
    clearColorRenderTarget(pRenderPass, pFrameParameter->pRenderTarget, 1.0f, 1.0f, 1.0f, 1.0f);
    bindGraphicsPipeline(pRenderPass, pTestData->pMaterial->pGraphicsPipeline);
    bindConstantBuffer(pRenderPass, pTestData->pSpinningCubeConstantBuffers[onlineBufferIndex], 0u, 0u);
    bindTextureSampler(pRenderPass, pTestData->pSampler, 0u, 1u);
    bindTexture(pRenderPass, pTestData->pTexture, 0u, 2u);
    drawIndexedMesh(pRenderPass, pTestData->pMesh, pTestData->pMaterial);

    endRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);   
    executeRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);

    setRenderPassDependency(pFrameParameter->pGraphicsFrame, pRenderPass, pComputePass);
}

bool initComputeTextureSample(sample_frame_parameter_t* pFrameParameter)
{
    compute_texture_test_data_t* pTestData = (compute_texture_test_data_t*)allocateFromAllocator(pFrameParameter->pAllocator, sizeof(compute_texture_test_data_t), alloc_flags_t::clear_memory);
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

    material_t* pMaterial = createMaterial(pFrameParameter->pGraphicsFrame, "Compute Texture Material", pFrameParameter->pAllocator, pMesh->pVertexFormat, computeTextureVertexShader, computeTexturePixelShader);
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

    viewMatrix.m23 = 2.0f;

    texture_sampler_parameter_t samplerParameter = {};
    samplerParameter.mipmapFilter = texture_sampler_filter_type_t::linear;
    samplerParameter.minifactionFilter = texture_sampler_filter_type_t::linear;
    samplerParameter.magnificationFilter = texture_sampler_filter_type_t::linear;
    samplerParameter.addressModeU = texture_sampler_address_mode_type_t::clamp;
    samplerParameter.addressModeV = texture_sampler_address_mode_type_t::clamp;
    samplerParameter.addressModeW = texture_sampler_address_mode_type_t::clamp;
    pTestData->pSampler = createTextureSampler(pFrameParameter->pGraphicsFrame, &samplerParameter);

    const uint32_t textureWidth = 256;
    const uint32_t textureHeight = 256;
    pTestData->spinningCubeData.viewMatrix = viewMatrix;
    pTestData->spinningCubeData.projMatrix = projectionMatrix;
    pTestData->spinningCubeData.viewProjMatrix = mulMatrices(&viewMatrix, &projectionMatrix);
    pTestData->textureHeight = textureHeight;
    pTestData->textureWidth = textureWidth;
    pTestData->computeTextureData.textureWidth = textureWidth;
    pTestData->pComputeConstBuffers[0] = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(compute_texture_data_t), &pTestData->computeTextureData, gpu_buffer_usage_flag_t::constant_buffer, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Compute Texture Data");
    pTestData->pComputeConstBuffers[1] = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(compute_texture_data_t), &pTestData->computeTextureData, gpu_buffer_usage_flag_t::constant_buffer, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Compute Texture Data");
    pTestData->pSpinningCubeConstantBuffers[0] = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(spinning_cube_constant_buffer_data_t), nullptr, gpu_buffer_usage_flag_t::constant_buffer, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Spinning Cube Constant Buffer");
    pTestData->pSpinningCubeConstantBuffers[1] = createGpuBuffer(pFrameParameter->pGraphicsFrame, sizeof(spinning_cube_constant_buffer_data_t), nullptr, gpu_buffer_usage_flag_t::constant_buffer, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Spinning Cube Constant Buffer");
    pTestData->pTexture = createGpuTexture(pFrameParameter->pGraphicsFrame, createUint3(textureWidth, textureHeight, 1), 1u, nullptr, gpu_texture_usage_flag_t::shader_resource_view, gpu_texture_format_t::R8G8B8A8, gpu_texture_format_type_t::normalized_unsigned_int, "Procedural Texture");
    pTestData->pComputeTextureBuffer = createGpuBuffer(pFrameParameter->pGraphicsFrame, textureWidth * textureHeight * 4, nullptr, gpu_buffer_usage_flag_t::storage_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess, "ComputeTextureBuffer" );
    pTestData->pComputeShader = compileShaderCode(pFrameParameter->pGraphicsFrame, computeTextureComputeShader, getStringLength(computeTextureComputeShader), "main", nullptr, "Generate Texture Shader", shader_type_t::compute_shader);
    pTestData->pComputePipeline = createComputePipeline(pFrameParameter->pGraphicsFrame, pTestData->pComputeShader, "ComputeTexture");
    pTestData->pMesh = pMesh;
    pTestData->pMaterial = pMaterial;

    pFrameParameter->pUserData = pTestData;
    return true;
}

void shutdownComputeTextureSample(sample_frame_parameter_t* pFrameParameter)
{
    compute_texture_test_data_t* pTestData = (compute_texture_test_data_t*)pFrameParameter->pUserData;
    releaseGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffers[0]);
    releaseGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pSpinningCubeConstantBuffers[1]);
    releaseGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pComputeConstBuffers[0]);
    releaseGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pComputeConstBuffers[1]);
    releaseGpuBuffer(pFrameParameter->pGraphicsFrame, pTestData->pComputeTextureBuffer);
    releaseGpuTexture(pFrameParameter->pGraphicsFrame, pTestData->pTexture);
    releaseTextureSampler(pFrameParameter->pGraphicsFrame, pTestData->pSampler);
    releaseShaderBinary(pFrameParameter->pGraphicsFrame, pTestData->pComputeShader);
    releaseComputePipeline(pFrameParameter->pGraphicsFrame, pTestData->pComputePipeline);
    destroyMaterial(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMaterial);
    destroyIndexedMesh(pFrameParameter->pGraphicsFrame, pFrameParameter->pAllocator, pTestData->pMesh);
    freeFromAllocator(pFrameParameter->pAllocator, pTestData);
}