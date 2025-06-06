
#include "generic.hlsli"

struct VOut
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float2 tc : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
    [[vk::location(3)]] float3 normal : NORMAL0;
    [[vk::location(4)]] float3 positionOS : POSITIONOS0;
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
    [[vk::location(1)]] float2 tc : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
    [[vk::location(3)]] float4 normal : NORMAL0;
};



VOut vs(VIn input)
{
    VOut output;
	output.position = mul(sceneView.projectionMatrix, mul(rc.modelViewMatrix, float4(input.position.xyz, 1.0)));
    output.positionOS = input.position.xyz;
    output.tc = input.tc;
    output.color = input.color;
    output.normal = input.normal.xyz;

    return output;
}

#endif

#if PS

struct RootConstants
{
    [[vk::offset(64)]]
	float3 lightPos;
	float lightRadius;
	uint lightColor;
	uint textureIndex;
	uint samplerIndex;
};
[[vk::push_constant]] RootConstants rc;

#include "game_textures.hlsli"

[earlydepthstencil]
float4 ps(VOut input) : SV_Target
{
    
    float4 color = input.color * texture[rc.textureIndex].Sample(mySampler[rc.samplerIndex], input.tc);
    float4 lightColor = unpackColorR11G11B10(rc.lightColor);
    float3 lv = rc.lightPos - input.positionOS;
    float dist = length(lv);
    float3 l = lv / dist;
    float intensity = max(1 - dist / rc.lightRadius, 0.0);
    float ndotl = max(dot(input.normal, l), 0.0);
    float frontside = max(sign(ndotl), 0.0);
    return color * lightColor * intensity * frontside;
}

#endif