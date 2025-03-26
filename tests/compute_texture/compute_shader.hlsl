cbuffer TextureGenBuffer : register(b0, space0)
{
    uint frameIndex;
    uint textureWidth;
    uint textureHeight;
};

RWStructuredBuffer<uint> textureBuffer : register(u0, space1);

[numthreads(8,8,1)]
void main( uint3 dispatchId : SV_DispatchThreadID )
{
    uint textureBlockX = dispatchId.x * 8;
    uint textureBlockY = dispatchId.y * 8;

    uint color = 0xFFFFFFFF;
    if( ( dispatchId.x & 1 ) == 0 )
    {
        color = 0x000000FF;
    }

    for(uint y = 0; y < textureBlockY; ++y)
    {
        for(uint x = 0; x < textureBlockX; ++x)
        {
            textureBuffer[x + y * textureWidth] = color;
        }
    }
}