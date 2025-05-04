struct RootConstants
{
    uint dstIndex; 
    uint clampMode;
    uint srcWidth;
    uint srcHeight;
};

[[vk::push_constant]] RootConstants rc;

#include "mipmap.hlsli"


[[vk::binding(1)]] RWTexture2D<float4> src;



[numthreads(8,8,1)]
void cs(uint3 dtID : SV_DispatchThreadID) 
{
    RWTexture2D<unorm float4> dst = mips[rc.dstIndex];
    uint dstWidth;
    uint dstHeight;
    dst.GetDimensions(dstWidth, dstHeight);
    if(dtID.x >= dstWidth || dtID.y >= dstHeight){
        return;
    }


    uint2 srcDims = uint2(rc.srcWidth, rc.srcHeight);
    
    float4 a = src[TexTC(uint2(dtID.x, dtID.y * 2), srcDims, rc.clampMode != 0)];
    float4 b = src[TexTC(uint2(dtID.x, dtID.y * 2 + 1), srcDims, rc.clampMode != 0)];

    dst[dtID.xy] = 0.5 * (a + b);
}
