struct PixelInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PixelInput input) : SV_Target
{
    return input.color;
}