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