
Texture2DMS<unorm float4> src;
RWTexture2D<unorm float4> dst;

[numthreads(8,8,1)]
void cs(uint3 dtID : SV_DispatchThreadID) 
{
    uint dstWidth;
    uint dstHeight;
    dst.GetDimensions(dstWidth, dstHeight);
    if(dtID.x >= dstWidth || dtID.y >= dstHeight){
        return;
    }
    
    float4 accum = float4(0, 0, 0, 0);
    for(int i = 0; i < 8; i++){
        accum += src.Load(dtID.xy, i);
    }

    dst[dtID.xy] = accum / 8.0;
}
