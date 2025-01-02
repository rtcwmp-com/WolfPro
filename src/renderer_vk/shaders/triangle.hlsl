//.\dxc.exe -spirv -T vs_6_0 -E vs C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_vs.h -Vn triangle_vs
//.\dxc.exe -spirv -T ps_6_0 -E ps C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_ps.h -Vn triangle_ps
struct VOut
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 tc : TEXCOORD0;
};

struct VIn
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 tc : TEXCOORD0;
};

VOut vs(VIn input)
{
    VOut output;
    output.position = input.position;
    output.color = input.color;
    output.tc = input.tc;

    return output;
}

Texture2D texture;
SamplerState mySampler;

float4 ps(VOut input) : SV_Target
{
    float4 texColor = texture.Sample(mySampler, input.tc);
    //float height, width;
    //texture.GetDimensions(width, height);
    //float4 texColor = texture.Load(int3(input.tc.x * width, input.tc.y * height, 0));
    
    return texColor;
    //return float4(input.tc,0,1);
    //return input.color;
}