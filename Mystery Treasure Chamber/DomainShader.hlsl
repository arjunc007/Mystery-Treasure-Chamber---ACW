//This shader for the floor takes a displacement map and displaces the flat tessellated quad to make a bumpy mesh

SamplerState txSampler : register(s0);
Texture2D txDisplacement : register(t0);

// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}; 

struct DS_OUTPUT
{
	float4 vPosition  : SV_POSITION;
	float2 Texture : TEXCOORD0;
	// TODO: change/add other stuff
};

// Output control point
struct HS_CONTROL_POINT_OUTPUT
{
	float4 vPosition : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]			: SV_TessFactor; // e.g. would be [4] for a quad domain
	float InsideTessFactor[2]			: SV_InsideTessFactor; // e.g. would be Inside[2] for a quad domain
	// TODO: change/add other stuff
};

#define NUM_CONTROL_POINTS 4

float4 BernsteinBasis(float t)
{
	float invT = 1.0f - t;
	return float4(invT * invT * invT,		//(1 - t)^3
		3.0f * t * invT * invT,				//3t * (1-t)^2
		3.0f * t * t * invT,				//3t^2 * (1 - t)
		t * t * t);							//t^3
}

float4 dBernsteinBasis(float t)
{
	float invT = 1.0f - t;

	return float4(-3 * invT * invT,
		3 * invT * invT - 6 * t * invT,
		6 * t * invT - 3 * t * t,
		3 * t * t);
}

float3 EvaluateBezier(const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> P, float4 U, float4 V)
{
	float3 Value = float3(0, 0, 0);
	Value = V.x * (P[0].vPosition * U.x + P[1].vPosition * U.y + P[2].vPosition * U.z + P[3].vPosition * U.w);
	Value += V.y * (P[4].vPosition * U.x + P[5].vPosition * U.y + P[6].vPosition * U.z + P[7].vPosition * U.w);
	Value += V.z * (P[8].vPosition * U.x + P[9].vPosition * U.y + P[10].vPosition * U.z + P[11].vPosition * U.w);
	Value += V.w * (P[12].vPosition * U.x + P[13].vPosition * U.y + P[14].vPosition * U.z + P[15].vPosition * U.w);

	return Value;
}

[domain("quad")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float2 UV : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT Output;

	float4 U = BernsteinBasis(UV.x);
	float4 V = BernsteinBasis(UV.y);

	float4 dU = dBernsteinBasis(UV.x);
	float4 dV = dBernsteinBasis(UV.y);

	//float3 WorldPos = EvaluateBezier(patch, U, V);
	//float3 Tangent = EvaluateBezier(patch, dU, V);
	//float3 BiNormal = EvaluateBezier(patch, U, dV);
	//float3 Norm = normalize(cross(Tangent, BiNormal));

	float3 vPos1 = (1.0 - UV.y)*patch[0].vPosition.xyz
			+ UV.y* patch[1].vPosition.xyz;
	float3 vPos2 = (1.0 - UV.y)*patch[2].vPosition.xyz
			+ UV.y* patch[3].vPosition.xyz;
	float3 uvPos = (1.0 - UV.x)*vPos1 + UV.x* vPos2;

	uvPos += 0.03f * txDisplacement.SampleLevel(txSampler, UV, 0).xyz * -patch[0].normal;

	Output.vPosition = mul(float4(uvPos, 1.0f), model);
	Output.vPosition = mul(Output.vPosition, view);
	Output.vPosition = mul(Output.vPosition, projection);

	Output.Texture = UV;

	return Output;
}
