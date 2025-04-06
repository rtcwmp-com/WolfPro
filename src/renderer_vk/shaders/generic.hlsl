//.\dxc.exe -spirv -T vs_6_0 -E vs C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_vs.h -Vn triangle_vs
//.\dxc.exe -spirv -T ps_6_0 -E ps C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_ps.h -Vn triangle_ps
#include "generic.hlsli"

struct VOut
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 tc : TEXCOORD0;
};

#if VS

struct RootConstants
{
	matrix modelViewMatrix;
};
[[vk::push_constant]] RootConstants rc;

struct VIn
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 tc : TEXCOORD0;
};



VOut vs(VIn input)
{
    VOut output;
	output.position = mul(sceneView.projectionMatrix, mul(rc.modelViewMatrix, float4(input.position.xyz, 1.0)));
    output.color = input.color;
    output.tc = input.tc;

    return output;
}

#endif

#if PS

struct RootConstants
{
    [[vk::offset(64)]]
	uint textureIndex;
    uint samplerIndex;
    uint alphaTest;
};
[[vk::push_constant]] RootConstants rc;

#include "game_textures.hlsli"


float4 ps(VOut input) : SV_Target
{
    
    float4 color = input.color * texture[rc.textureIndex].Sample(mySampler[rc.samplerIndex], input.tc);
    if(failsAlphaTest(rc.alphaTest, color.a)){
        discard;
    }
    return color;
}

#endif