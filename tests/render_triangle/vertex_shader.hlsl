struct VertexInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput output;
    output.pos = float4(vertexInput.pos, 1.0f);
    output.color = vertexInput.color;
    return output;
}