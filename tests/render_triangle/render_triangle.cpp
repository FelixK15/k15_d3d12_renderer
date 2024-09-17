
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
    pMesh->pVertexFormat = pVertexFormat;
    pMesh->pVertexBuffer = pMeshVertexBuffer;

    return pMesh;
}

D3D12_BLEND_DESC createDefaultBlendDesc()
{
    D3D12_BLEND_DESC defaultBlendDesc = {};
    defaultBlendDesc.RenderTarget[0].BlendEnable            = FALSE;
    defaultBlendDesc.RenderTarget[0].LogicOpEnable          = FALSE;
    defaultBlendDesc.RenderTarget[0].SrcBlend               = D3D12_BLEND_ONE;
    defaultBlendDesc.RenderTarget[0].DestBlend              = D3D12_BLEND_ZERO;
    defaultBlendDesc.RenderTarget[0].BlendOp                = D3D12_BLEND_OP_ADD;
    defaultBlendDesc.RenderTarget[0].SrcBlendAlpha          = D3D12_BLEND_ONE;
    defaultBlendDesc.RenderTarget[0].DestBlendAlpha         = D3D12_BLEND_ZERO;
    defaultBlendDesc.RenderTarget[0].BlendOpAlpha           = D3D12_BLEND_OP_ADD;
    defaultBlendDesc.RenderTarget[0].LogicOp                = D3D12_LOGIC_OP_NOOP;
    defaultBlendDesc.RenderTarget[0].RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL;

    return defaultBlendDesc;
}

D3D12_DEPTH_STENCIL_DESC createDefaultDepthStencilDesc()
{
    D3D12_DEPTH_STENCIL_DESC defaultDepthStencilState = {};
    defaultDepthStencilState.DepthEnable                    = TRUE;
    defaultDepthStencilState.DepthWriteMask                 = D3D12_DEPTH_WRITE_MASK_ALL;
    defaultDepthStencilState.DepthFunc                      = D3D12_COMPARISON_FUNC_LESS;
    defaultDepthStencilState.StencilEnable                  = FALSE;
    defaultDepthStencilState.StencilReadMask                = D3D12_DEFAULT_STENCIL_READ_MASK;
    defaultDepthStencilState.StencilWriteMask               = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    defaultDepthStencilState.FrontFace.StencilFailOp        = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilState.FrontFace.StencilDepthFailOp   = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilState.FrontFace.StencilPassOp        = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilState.FrontFace.StencilFunc          = D3D12_COMPARISON_FUNC_ALWAYS;
    defaultDepthStencilState.BackFace.StencilFailOp         = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilState.BackFace.StencilDepthFailOp    = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilState.BackFace.StencilPassOp         = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilState.BackFace.StencilFunc           = D3D12_COMPARISON_FUNC_ALWAYS;

    return defaultDepthStencilState;
}

D3D12_RASTERIZER_DESC createDefaultRasterizerDesc()
{
    D3D12_RASTERIZER_DESC defaultRasterizerDesc = {};
    defaultRasterizerDesc.FillMode              = D3D12_FILL_MODE_SOLID;
    defaultRasterizerDesc.CullMode              = D3D12_CULL_MODE_BACK;
    defaultRasterizerDesc.FrontCounterClockwise = FALSE;
    defaultRasterizerDesc.DepthBias             = 0;
    defaultRasterizerDesc.DepthBiasClamp        = 0.0f;
    defaultRasterizerDesc.SlopeScaledDepthBias  = 0.0f;
    defaultRasterizerDesc.DepthClipEnable       = TRUE;
    defaultRasterizerDesc.MultisampleEnable     = FALSE;
    defaultRasterizerDesc.AntialiasedLineEnable = FALSE;
    defaultRasterizerDesc.ForcedSampleCount     = 0;
    defaultRasterizerDesc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    return defaultRasterizerDesc;
}

graphics_pipeline_state_t* createGraphicsPipelineState(graphics_frame_t* pGraphicsFrame, const graphics_pipeline_state_parameters_t* pPipelineStateParameters)
{
    graphics_pipeline_state_t* pPipelineState = allocatePipelineState(pGraphicsFrame->pRenderResourceCache);
    if(pPipelineState == nullptr)
    {
        return nullptr;
    }

/*
    LPCSTR SemanticName;
    UINT SemanticIndex;
    DXGI_FORMAT Format;
    UINT InputSlot;
    UINT AlignedByteOffset;
    D3D12_INPUT_CLASSIFICATION InputSlotClass;
    UINT InstanceDataStepRate;*/

    D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc = {};
    graphicsPipelineStateDesc.VS.BytecodeLength     = pPipelineStateParameters->pVertexShader->shaderBlobSizeInBytes;
    graphicsPipelineStateDesc.VS.pShaderBytecode    = pPipelineStateParameters->pVertexShader->pShaderBlob;
    graphicsPipelineStateDesc.PS.BytecodeLength     = pPipelineStateParameters->pPixelShader->shaderBlobSizeInBytes;
    graphicsPipelineStateDesc.PS.pShaderBytecode    = pPipelineStateParameters->pPixelShader->pShaderBlob;
    graphicsPipelineStateDesc.NumRenderTargets      = 0u;
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineStateDesc.SampleDesc.Count      = 1u;
    graphicsPipelineStateDesc.SampleDesc.Quality    = 0u;
    graphicsPipelineStateDesc.BlendState            = createDefaultBlendDesc();
    graphicsPipelineStateDesc.DepthStencilState     = createDefaultDepthStencilDesc();
    graphicsPipelineStateDesc.RasterizerState       = createDefaultRasterizerDesc();

    graphicsPipelineStateDesc.InputLayout.NumElements = 1;
    graphicsPipelineStateDesc.InputLayout.pInputElementDescs = elementDescs;

    ID3D12PipelineState* pPipelineStateObject = nullptr;
    const HRESULT pipelineStateObjectResult = pGraphicsFrame->pDevice->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pPipelineStateObject));
    if(pipelineStateObjectResult != S_OK)
    {
        logError("'%s' while trying to create graphics pipeline state '%s'.", getHResultString(pipelineStateObjectResult), pPipelineStateParameters->pName);
        return nullptr;
    }

    setD3D12ObjectDebugName(pPipelineStateObject, pPipelineStateParameters->pName);
    
    pPipelineState->pPipelineState = pPipelineStateObject;
    return pPipelineState;
}

material_t* createMaterial(graphics_frame_t* pGraphicsFrame, vertex_format_t* pVertexFormat, const shader_compilation_parameters_t* pVertexShaderParameters, const shader_compilation_parameters_t* pPixelShaderParameters)
{
    graphics_pipeline_state_parameters_t pipelineStateParameters = {};
    pipelineStateParameters.pVertexShader   = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pVertexShaderParameters);
    pipelineStateParameters.pPixelShader    = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pPixelShaderParameters);
    pipelineStateParameters.pVertexFormat   = pVertexFormat;

    graphics_pipeline_state_t* pDefaultPipelineStateObject = createGraphicsPipelineState(pGraphicsFrame, &pipelineStateParameters);
    material_t* pMaterial = (material_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, sizeof(material_t));
    pMaterial->pGraphicsPipelineState = pDefaultPipelineStateObject;
    return pMaterial;
}

mesh_t* createSingleTriangleMesh(graphics_frame_t* pGraphicsFrame)
{
    const float triangleVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f
    };

    //render_pass_t* pDrawRenderPass = startRenderPass(pGraphicsFrame, "Draw Triangle");
    vertex_attribute_entry_t pVertexAttributes[] = {
        {vertex_attribute_t::position, vertex_attribute_type_t::float32, 3u}
    };

    vertex_format_t* pVertexFormat = createVertexFormat(pGraphicsFrame, pVertexAttributes, 1u);

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

    mesh_t* pMesh = createSingleTriangleMesh(pGraphicsFrame);
    material_t* pMaterial = createMaterial(pGraphicsFrame, pMesh->pVertexFormat, &vs_para, &ps_para);

    const float r = (float)cursorPos.x / (float)(clientRect.right - clientRect.left);
    const float g = (float)cursorPos.y / (float)(clientRect.bottom - clientRect.top);

    const FLOAT backBufferRGBA[4] = {r, g, 0.2f, 1.0f};

    render_pass_t* pRenderPass = startRenderPass(pGraphicsFrame, "Draw Triangle");
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