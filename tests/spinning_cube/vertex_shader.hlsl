cbuffer SpinningCubeData : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjMatrix2;
    float4x4 modelMatrix;
};

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
    float4x4 viewProjMatrix = mul(viewMatrix, projMatrix);
    float4x4 modelViewProjMatrix = mul(modelMatrix, viewProjMatrix);

    VertexOutput output;
    output.pos = mul(vertexInput.pos, modelViewProjMatrix);
    output.color = float4(vertexInput.color, 1.0f);
    return output;
}