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

cbuffer ConstantBuffer : register(b2)
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
	float3 normal : NORMAL;
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	float4 Pos = float4(Input.pos, 1.0f);
	
	//Fix aspect ratio
	//Pos.x *= (height / width);

	//Animate snake with time
	float t = 4 + 0.5 * (sin(time));
	//if (Pos.z > -1.f)
		Pos.x += 0.07f * sin(5 * Pos.z * t);

	Output.Position = mul(Pos, model);
	Output.Position = mul(Output.Position, view);
	Output.Position = mul(Output.Position, projection);
	Output.Texture = Input.tex;
	Output.normal = -Input.norm;

	return(Output);

}



