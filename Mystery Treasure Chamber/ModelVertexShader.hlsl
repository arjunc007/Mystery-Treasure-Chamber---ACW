// This vertex shader simply transforms mesh local to world positions
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer ChangesOnResizeConstantBuffer : register(b1)
{
	float height;
	float width;
	float2 padding;
}

cbuffer TimeBuffer : register(b2)
{
	float time;
	float3 padding2;
}

// Per-vertex data used as input to the vertex shader.
struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 norm : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 Texture : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
	float3 normal : NORMAL;
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	float4 Pos = float4(Input.pos, 1.0f);
	
	//Fix aspect ratio
	//Pos.x *= (height / width);

	//Animate snake with time
    float waveSpeed = 5.0f;
    float waveFrequency = 2.0f;
    float waveAmplitude = 0.07f;
	
    float headZ = -0.42f;
    float neckZ = 0.25f;
	
    float slitherMask = 1.0f - smoothstep(neckZ, headZ, Pos.z);
    float rawSlither = sin(Pos.z * waveFrequency + time * waveSpeed) * waveAmplitude;
	
    float finalSlitherOffset = rawSlither * slitherMask;
    Pos.x += finalSlitherOffset;

	Output.Position = mul(Pos, model);
	Output.Position = mul(Output.Position, view);
	Output.Position = mul(Output.Position, projection);
	Output.Texture = Input.tex;
    Output.normal = normalize(mul(Input.norm, (float3x3)model));
    Output.WorldPos = mul(Pos, model).xyz;

	return(Output);

}



