
[[vk::binding(0)]] RWTexture2D<unorm float4> mips[12];

int2 TexTC(int2 tc, int2 dims, bool clampMode){
    int2 maxTC = dims - int2(1,1);
    if(clampMode){
        return clamp(tc, int2(0,0), maxTC);
    }else{
        return (tc + 16 * dims) & maxTC;
    }
    
}