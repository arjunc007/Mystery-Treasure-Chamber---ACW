// Per-vertex data used as input to the vertex shader.
struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 norm : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
};

struct HS_INPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

HS_INPUT main( VS_INPUT input )
{
	HS_INPUT output = (HS_INPUT)0;

	output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	output.normal = input.norm;

	return output;
}