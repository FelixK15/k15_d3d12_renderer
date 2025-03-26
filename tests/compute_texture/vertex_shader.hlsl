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