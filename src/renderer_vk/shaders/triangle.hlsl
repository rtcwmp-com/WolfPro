//.\dxc.exe -spirv -T vs_6_0 -E vs C:\Users\Ryan\Documents\GitHub\RTCW-MP\src\renderer_vk\shaders\triangle.hlsl -Fh triangle_vs.h -Vn triangle_vs
struct VOut
{
    float4 position : SV_Position;
};

// static const float2 g_position[3] =
// {
//     float2(-0.7, 0.7),
//     float2(0.7, 0.7),
//     float2(0.0, -0.7)
// };

struct VIn
{
    float4 position : SV_Position;
};

VOut vs(VIn input)
{
    VOut output;
    output.position = input.position;

    return output;
}

float4 ps(VOut input) : SV_Target
{
    return float4(0.5, 1.0, 0.5, 1.0);
}