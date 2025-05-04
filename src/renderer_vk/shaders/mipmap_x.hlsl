struct RootConstants
{
	uint srcIndex;
    uint clampMode;
    uint dstWidth;
    uint dstHeight;
};

[[vk::push_constant]] RootConstants rc;

#include "mipmap.hlsli"

[[vk::binding(1)]] RWTexture2D<float4> dst;



[numthreads(8,8,1)]
void cs(uint3 dtID : SV_DispatchThreadID) 
{
    //RWTexture2D<unorm float4> dst = mips[12];
    if(dtID.x >= rc.dstWidth || dtID.y >= rc.dstHeight){
        return;
    }
    RWTexture2D<unorm float4> src = mips[rc.srcIndex];
    uint srcWidth;
    uint srcHeight;
    src.GetDimensions(srcWidth, srcHeight);

    uint2 srcDims = uint2(srcWidth, srcHeight);
    
    float4 a = src[TexTC(uint2(dtID.x * 2, dtID.y), srcDims, rc.clampMode != 0)];
    float4 b = src[TexTC(uint2(dtID.x * 2 + 1, dtID.y), srcDims, rc.clampMode != 0)];

    dst[dtID.xy] = 0.5 * (a + b);
}
