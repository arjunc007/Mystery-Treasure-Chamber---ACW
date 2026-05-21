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

float3 RandDirCone(float3 pos)
{
    float2 uv = float2(totalTime + pos.x, totalTime + pos.z) * 0.1f;
	
    float3 noise = txTexture.SampleLevel(txSampler, uv, 0).rgb;
	
    float spreadX = noise.x - 0.5f;
    float spreadZ = noise.z - 0.5f;
	
    float upwardForce = noise.y + 1.0f;

    return normalize(float3(spreadX, upwardForce, spreadZ));
}

float RandomFloat(float3 pos, float min, float max)
{
    float2 uv = float2(totalTime - pos.x, pos.z - totalTime) * 0.15f;
	
    float r = txTexture.SampleLevel(txSampler, uv, 0).x;

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
			
            float3 randomDir = RandDirCone(p.position);
            pNew.speed = RandomFloat(p.position, 1.0f, 4.0f) * randomDir;
			pNew.size = p.size;
			
			output.Append(pNew);

			//Reset age
			p.age = 0.0f;
		}
	}
	else
	{
        float lifetime = 0.5f;
        p.age += deltaTime;
		//Keep particle if living
		if (p.age <= lifetime)
        {
            p.position += p.speed * deltaTime;
			
			//Size depends on age
            float lifeRatio = p.age / lifetime;
            p.size = lerp(float2(1.0f, 1.0f), float2(0.1f, 0.1f), lifeRatio);
			
			output.Append(p);
        }
	}
}