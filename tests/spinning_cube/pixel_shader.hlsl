SamplerState mySampler : register(s0, space1);
Texture2D<float> texture : register(t0, space2);

struct PixelInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

float4 main(PixelInput input) : SV_Target
{
    float4 bla = texture.Sample(mySampler, float2(1.0f, 1.0f));
    return float4(input.uv, bla.x, 1.0f);
}   