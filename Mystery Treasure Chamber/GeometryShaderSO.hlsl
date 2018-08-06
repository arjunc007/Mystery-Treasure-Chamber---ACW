//Geometry shader for calculating new particle creation or destruction
//Streams output to vertex buffer, not to pixel shader

Texture2D txTexture : register(t0);
SamplerState txSampler : register(s0);

cbuffer ConstantBuffer : register(b0)
{
	float totalTime;
	float deltaTime;
	float2 padding;
}

struct Particle {
	float3 position : POSITION;
	float3 speed : TEXCOORD0;
	float2 size : TEXCOORD1;
	float age : TEXCOORD2;
	uint type : TEXCOORD3;
};

float3 RandDirCone(float offset)
{
	float u = totalTime + offset;
	float3 v = txTexture.SampleLevel(txSampler, u, 0).xyz;

	return normalize(v);
}

float RandomFloat(float min, float max)
{
	//Between 0 and 1
	float r = (txTexture.SampleLevel(txSampler, totalTime, totalTime).x + 1) / 2;

	return r * (max - min) + min;

}

[maxvertexcount(2)]
void main(
	point Particle input[1],
	inout PointStream< Particle > output)
{
	input[0].age += deltaTime;

	//Particle is emitter
	if (input[0].type == 0)
	{
		if (input[0].age > 0.005f)
		{
			float3 randomDir = RandDirCone(0.0f);

			Particle p;
			p.position = input[0].position;
			p.speed = RandomFloat(1.0f, 5.0f) * randomDir;
			p.size = input[0].size;
			p.age = 0.0f;
			p.type = 1;

			output.Append(p);

			//Reset age
			input[0].age = 0.0f;
		}

		//Keep the emitter
		output.Append(input[0]);
	}
	else
	{
		//Keep particle if living
		if (input[0].age <= 1.0f)
			output.Append(input[0]);
	}
}