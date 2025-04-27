struct RootConstants
{
	uint srcIndex;
    uint dstIndex;
    uint clampMode;
};

[[vk::push_constant]] RootConstants rc;

#include "mipmap.hlsli"

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
    RWTexture2D<unorm float4> src = mips[rc.srcIndex];
    uint srcWidth;
    uint srcHeight;
    src.GetDimensions(srcWidth, srcHeight);

    uint2 srcDims = uint2(srcWidth, srcHeight);
    
    float4 a = src[TexTC(dtID.xy * 2 + uint2(0,0), srcDims)];
    float4 b = src[TexTC(dtID.xy * 2 + uint2(0,1), srcDims)];
    float4 c = src[TexTC(dtID.xy * 2 + uint2(1,0), srcDims)];
    float4 d = src[TexTC(dtID.xy * 2 + uint2(1,1), srcDims)];

    dst[dtID.xy] = 0.25 * (a + b + c + d);
}
