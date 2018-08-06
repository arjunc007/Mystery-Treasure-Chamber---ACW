//Texture shader for the particles

Texture2D txTexture : register(t0);
SamplerState txSampler : register(s0);

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 color: COLOR0;
	float2 uv: TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	return input.color * txTexture.Sample(txSampler, input.uv);
}