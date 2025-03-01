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
}