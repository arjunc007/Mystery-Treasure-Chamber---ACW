//Geometry shader for calculating new particle creation or destruction
//Streams output to vertex buffer, not to pixel shader

Texture2D txTexture : register(t0);
SamplerState txSampler : register(s0);

cbuffer TimeBuffer : register(b0)
{
	float totalTime;
	float deltaTime;
	float2 padding;
}

struct Particle {
    float3 position : TEXCOORD0;
	float3 speed : TEXCOORD1;
	float2 size : TEXCOORD2;
	float age : TEXCOORD3;
	uint type : TEXCOORD4;
};

float3 RandDirCone(float offset)
{
    float u = (totalTime + offset) * 0.05f;
	float3 v = txTexture.SampleLevel(txSampler, u, 0).xyz;

	return normalize(v);
}

float RandomFloat(float min, float max)
{
	//Between 0 and 1
	float r = (txTexture.SampleLevel(txSampler, totalTime, totalTime).x + 1) / 2;

	return r * (max - min) + min;

}

[maxvertexcount(6)]
void main(
	point Particle input[1],
	inout PointStream< Particle > output)
{
    Particle p = input[0];

	//Particle is emitter
	if (p.type == 0)
	{
        p.age += deltaTime;
        output.Append(p);
		
		if (p.age > 0.05f)
		{
            Particle pNew;
            pNew.type = 1;
            pNew.age = 0.0f;
			
            pNew.position = p.position;
			
			float3 randomDir = RandDirCone(p.position.y);
			pNew.speed = RandomFloat(1.0f, 5.0f) * randomDir;
			
			pNew.size = p.size;
			
			output.Append(pNew);

			//Reset age
			p.age = 0.0f;
		}
	}
	else
	{
        p.age += deltaTime;
		//Keep particle if living
		if (p.age <= 1.0f)
        {
            p.position += p.speed * deltaTime;
			
			//Size depends on age
            float lifeRatio = p.age / 1.0f;
            p.size = lerp(float2(1.0f, 1.0f), float2(0.1f, 0.1f), lifeRatio);
			
			output.Append(p);
        }
	}
}