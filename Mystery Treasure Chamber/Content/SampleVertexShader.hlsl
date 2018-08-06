// A constant buffer that stores the three basic column-major matrices for composing geometry.
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

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 color : COLOR0;
};

//Canvas
struct VS_Canvas
{
	float4 Position : SV_POSITION;	//vertex position
	float2 canvasXY : TEXCOORD0;	//vertex texture coordinates
	float2 tex : TEXCOORD1;
};

// Simple shader to do vertex processing on the GPU.
VS_Canvas main(VertexShaderInput input)
{
	VS_Canvas Output;

	Output.Position = float4(sign(input.pos.xy), 0, 1);

	float2 canvasSize = float2(width, height);
	Output.canvasXY = sign(input.pos.xy) * canvasSize;
	Output.tex = float2((Output.Position.x + 1) / 2, (Output.Position.y - 1) / -2);

	return Output;
}