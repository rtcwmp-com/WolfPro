struct RootConstants
{
	uint srcIndex;
    uint dstIndex;
};

[[vk::push_constant]] RootConstants rc;

[[vk::binding(0)]] RWTexture2D<unorm float4> mips[12];

[numthreads(8,8,1)]
void cs(uint3 dtID : SV_DispatchThreadID) 
{
	RWTexture2D<unorm float4> src = mips[rc.srcIndex];
    RWTexture2D<unorm float4> dst = mips[rc.dstIndex];
    
    float4 a = src[dtID.xy * 2 + uint2(0,0)];
    float4 b = src[dtID.xy * 2 + uint2(0,1)];
    float4 c = src[dtID.xy * 2 + uint2(1,0)];
    float4 d = src[dtID.xy * 2 + uint2(1,1)];

    dst[dtID.xy] = 0.25 * (a + b + c + d);
}
