
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
	output.texCoords.y = (float)(id % 2) * 2.0;

	return output;
}

#endif


#if PS


[[vk::binding(0)]] Texture2D texture0;
[[vk::binding(1)]] SamplerState sampler0;

float4 ps(VOut input) : SV_Target
{
	float3 base = texture0.Sample(sampler0, input.texCoords).rgb;

	return float4(base, 1.0);
}

#endif