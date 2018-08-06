//Move the particles according to their age
struct VS_INPUT {
	float3 position : POSITION;
	float3 speed : TEXCOORD0;
	float2 size : TEXCOORD1;
	float age : TEXCOORD2;
	uint type : TEXCOORD3;
};

struct GS_INPUT
{
	float3 position : POSITION;
	float2 size : TEXCOORD0;
	float alpha : TEXCOORD1;
	uint type : TEXCOORD2;
};

GS_INPUT main( VS_INPUT input )
{
	GS_INPUT output;

	float t = input.age;
	output.position = input.position + t * input.speed + 0.5f * 1.0f * t *t;

	output.alpha = 1.0f - smoothstep(0.0f, 1.0f, t);
	
	output.size = input.size;
	output.type = input.type;

	return output;
}