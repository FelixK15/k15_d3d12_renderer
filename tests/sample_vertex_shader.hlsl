struct VertexInput
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput output;
    output.pos = float4(vertexInput.pos.x, vertexInput.pos.y, 0.0f, 1.0f);
    output.uv = vertexInput.uv;
    return output;
}