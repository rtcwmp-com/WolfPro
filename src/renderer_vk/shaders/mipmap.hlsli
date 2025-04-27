
[[vk::binding(0)]] RWTexture2D<unorm float4> mips[12];

uint2 TexTC(uint2 tc, uint2 dims){
    return min(tc, dims - uint2(1,1));
}