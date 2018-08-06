//Draws the particle by making quads from input points in such a way to always face the camera
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
	float2 pad0;
}

cbuffer ViewBuffer : register(b2)
{
	float4 Eye;
	float4 LightColor;
	float4 backgroundColor;
	float4 LightPos[3];
	float nearPlane;
	float farPlane;
	float2 padding;
};

struct GS_INPUT
{
	float3 position : POSITION;
	float2 size : TEXCOORD0;
	float alpha : TEXCOORD1;
	uint type : TEXCOORD2;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 color: COLOR0; 
	float2 uv: TEXCOORD0;
};

static const float2 quadTex[4] = {
	{1, 1},
	{1, 0},
	{0, 0},
	{0, 1}
};

[maxvertexcount(4)]
void main(
	point GS_INPUT input[1], 
	inout TriangleStream< PS_INPUT > output)
{
	if (input[0].type != 0)
	{
		float3 look = normalize(Eye.xyz - input[0].position.xyz);
		float3 right = cross(float3(0, 1, 0), look);
		float3 up = cross(look, right);

		float4 v[4];
		v[0] = float4(input[0].position + right * input[0].size.x * height / width - input[0].size.y * up, 1.0f);
		v[1] = float4(input[0].position + right * input[0].size.x * height / width + input[0].size.y * up, 1.0f);
		v[2] = float4(input[0].position - right * input[0].size.x * height / width - input[0].size.y * up, 1.0f);
		v[3] = float4(input[0].position - right * input[0].size.x * height / width + input[0].size.y * up, 1.0f);

		PS_INPUT p;
		[unroll]
		for (int i = 0; i < 4; ++i)
		{
			p.pos = mul(v[i], model);
			p.pos = mul(p.pos, view);
			p.pos = mul(p.pos, projection);
			p.uv = quadTex[i];
			p.color = float4(1.0f, 1.0f, 1.0f, input[0].alpha);
			output.Append(p);
		}

	}
}