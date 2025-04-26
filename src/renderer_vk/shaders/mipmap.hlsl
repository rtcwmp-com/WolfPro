struct RootConstants
{
	uint srcIndex;
    uint dstIndex;
};

[[vk::push_constant]] RootConstants rc;

[[vk::binding(0)]] RWTexture2D<unorm float4> mips[12];

[numthreads(8,8,1)]
void cs()
{
	
}
