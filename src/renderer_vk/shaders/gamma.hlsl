
struct VOut
{
	float4 position : SV_Position;
	float2 texCoords : TEXCOORD0;
};


#if VS

VOut vs(uint id : SV_VertexID)
{
	VOut output;
	output.position.x = (float)(id / 2) * 4.0 - 1.0;
	output.position.y = (float)(id % 2) * 4.0 - 1.0;
	output.position.z = 0.0;
	output.position.w = 1.0;
	output.texCoords.x = (float)(id / 2) * 2.0;
	output.texCoords.y = 1.0 - (float)(id % 2) * 2.0;

	return output;
}

#endif


#if PS

// X3571: pow(f, e) won't work if f is negative
#pragma warning(disable : 3571)

cbuffer RootConstants
{
	float invGamma;
	float brightness;
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 ps(VOut input) : SV_Target
{
	float3 base = texture0.Sample(sampler0, input.texCoords).rgb;
	float3 gc = pow(base, invGamma) * brightness;

	return float4(gc, 1.0);
}

#endif