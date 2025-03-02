//.\dxc.exe -spirv -T vs_6_0 -E vs C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_vs.h -Vn triangle_vs
//.\dxc.exe -spirv -T ps_6_0 -E ps C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_ps.h -Vn triangle_ps


struct VOut
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float2 tc : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
};

#if VS

struct RootConstants
{
    float2 scale;
    float2 bias;
};
[[vk::push_constant]] RootConstants rc;

struct VIn
{
    [[vk::location(0)]] float2 position : SV_Position;
    [[vk::location(1)]] float2 tc : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
};


VOut vs(VIn input)
{
    VOut output;
	output.position = float4(input.position.xy * rc.scale + rc.bias, 0.0, 1.0);
    output.color = input.color;
    output.tc = input.tc;

    return output;
}

#endif

#if PS

struct RootConstants
{
    [[vk::offset(16)]]
	uint textureIndex;
    uint samplerIndex;
};
[[vk::push_constant]] RootConstants rc;

[[vk::binding(0)]] Texture2D texture[2048];
[[vk::binding(1)]] SamplerState mySampler[6];

float4 ps(VOut input) : SV_Target
{
    float4 color = input.color * texture[rc.textureIndex].Sample(mySampler[rc.samplerIndex], input.tc);
    return color;
}

#endif