//.\dxc.exe -spirv -T vs_6_0 -E vs C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_vs.h -Vn triangle_vs
//.\dxc.exe -spirv -T ps_6_0 -E ps C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_ps.h -Vn triangle_ps
#include "generic.hlsli"

struct VOut
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float2 tc1 : TEXCOORD0;
    [[vk::location(2)]] float2 tc2 : TEXCOORD1;
    [[vk::location(3)]] float3 positionVS : POSITIONVS;
    [[vk::location(4)]] float3 positionOS : POSITIONOS;
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
    [[vk::location(1)]] float2 tc1 : TEXCOORD0;
    [[vk::location(2)]] float2 tc2 : TEXCOORD1;
};



VOut vs(VIn input)
{
    float4 positionVS = mul(rc.modelViewMatrix, float4(input.position.xyz, 1.0));
    VOut output;
	output.position = mul(sceneView.projectionMatrix, positionVS);
    output.positionVS = positionVS.xyz;
    output.positionOS = input.position.xyz;
    output.tc1 = input.tc1;
    output.tc2 = input.tc2;

    return output;
}

#endif

#if PS

struct RootConstants
{
    [[vk::offset(64)]]
	float3 lightPositionOS;
    float lightRadius;
    float3 lightColor;
    uint textureIndex1;
    uint samplerIndex1;
    uint textureIndex2;
    uint samplerIndex2;
    uint alphaTest;
    uint texEnv;
    
};
[[vk::push_constant]] RootConstants rc;

#include "game_textures.hlsli"

float4 texEnv(float4 p, float4 s, uint texEnv){
    if(texEnv == 100){ //MODULATE
        return p*s;
    }else if(texEnv == 101){ //ADD
        float3 c = p.rgb + s.rgb;
        float a = p.a * s.a;
        return float4(c,a);
    }else{ //DECAL
        float3 c = s.rgb * s.a + p.rgb * (1.0 - s.a);
        float a = p.a;
        return float4(c,a);
    }
}


[earlydepthstencil]
float4 ps(VOut input) : SV_Target
{
    float3 light = float3(0,0,0);
    // for(int i = 0; i < sceneView.lightCount; i++){
    //     DynamicLight dl = sceneView.lights[i];
    //     float d = distance(dl.position, input.positionVS);
    //     float intensity = saturate(1.0 - d/dl.radius);
    //     light += dl.color.rgb * intensity;
    // }
    if(rc.lightRadius > 0.0){
        float d = distance(rc.lightPositionOS, input.positionOS);
        float intensity = saturate(1.0 - d/rc.lightRadius);
        light += rc.lightColor * intensity;
    }
    
    float4 color1 = texture[rc.textureIndex1].Sample(mySampler[rc.samplerIndex1], input.tc1);
    float4 color2 = texture[rc.textureIndex2].Sample(mySampler[rc.samplerIndex2], input.tc2);
    return texEnv(color1, color2, rc.texEnv) + float4(light, 0);
}

#endif