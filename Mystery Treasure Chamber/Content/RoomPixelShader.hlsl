//This is for the room raymarching or the first raymarching pass

Texture2D txTexture : register(t0);
SamplerState txSampler : register(s0);

#define NUMLIGHTS 4

#define MIN_XYZ -5.0
#define MAX_XYZ 5.0
static const float3 BoxMinimum = (float3)MIN_XYZ;
static const float3 BoxMaximum = (float3)MAX_XYZ;
#define INTERVALS 200

static const float3 Zero = float3 (0.0, 0.0, 0.0);
static const float3 Unit = float3 (1.0, 1.0, 1.0);
static const float3 AxisX = float3 (1.0, 0.0, 0.0);
static const float3 AxisY = float3 (0.0, 1.0, 0.0);
static const float3 AxisZ = float3 (0.0, 0.0, 1.0);
#define STEP 0.01

cbuffer PixelShaderConstantBuffer : register(b0)
{
float4 Eye;
float4 LightColor;
float4 backgroundColor;
float4 LightPos[4];
float nearPlane;
float farPlane;
float2 padding;
};

struct Ray
{
	float3 o;	//origin
	float3 d;	//direction
};

//Canvas
struct VS_Canvas
{
	float4 Position : SV_POSITION;	//vertex position
	float2 canvasXY : TEXCOORD0;	//vertex texture coordinates
};

//------------------------------------------------------------------------------------------------------------------

float sdBox(float3 p, float3 b)
{
	float3 d = abs(p) - b;
	return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdPlane(float3 p, float4 n)
{
	// n must be normalized
	return dot(p, n.xyz) + n.w;
}

// SDF for a classic Diamond/Octahedron
float sdOctahedron(float3 p, float s)
{
    p = abs(p);
    return (p.x + p.y + p.z - s) * 0.57735027f; // Multiply by 1/sqrt(3) for exact distance
}

// SDF for a blocky Emerald/Ruby (Box)
float sdBox2(float3 p, float3 b)
{
    float3 d = abs(p) - b;
    return length(max(d, 0.0f)) + min(max(d.x, max(d.y, d.z)), 0.0f);
}

// A standard HLSL spatial hash function
float Hash3D(float3 p)
{
    p = frac(p * 0.3183099f + 0.1f);
    p *= 17.0f;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float2 opU(float2 d1, float2 d2)
{
    return (d1.x < d2.x) ? d1 : d2;
}

float room(float3 Position)
{
	return  -sdBox(Position, float3(4.95, 4.95, 4.95));
}

float2 MapTreasureHoard(float3 p)
{
    // the spacing between gems
    float spacing = 2.0f;
    
    // ID of the current grid cell
    float2 cellID = floor((p.xz + spacing * 0.5f) / spacing);
    float2 localXZ = fmod(abs(p.xz + spacing * 0.5f), spacing) - spacing * 0.5f;
    
    if (abs(cellID.x) > 2.0f || abs(cellID.y) > 2.0f)
    {
        return float2(100.0f, 0.0f);
    }
    
    // Generate a random number specific to this cell
    float randomVal = Hash3D(float3(cellID.x, 0.0f, cellID.y));
    
    // Randomize Size (e.g., between 0.2 and 0.6)
    float gemSize = lerp(0.2f, 0.6f, randomVal);
    
    float3 localPos = float3(localXZ.x, p.y + 1.0f - (gemSize * 0.5f), localXZ.y);
    // Randomize Position slightly within the cell
    // (So they don't look like a perfect chess board)
    localPos.x += (Hash3D(float3(cellID.x, 10.0f, cellID.y)) - 0.5f) * 0.8f;
    localPos.z += (Hash3D(float3(cellID.x, 20.0f, cellID.y)) - 0.5f) * 0.8f;
    
    // 7. Randomize the Shape!
    float2 result = (float2)0;
    if (randomVal > 0.5f)
    {
        // 50% chance to be a Diamond
        result = float2(sdOctahedron(localPos, gemSize), 2.0f);
    }
    else
    {
        // 50% chance to be an Emerald/Ruby
        // We use randomVal to make the box dimensions slightly rectangular
        float3 boxExtents = float3(gemSize, gemSize * 0.5f, gemSize * 0.8f);
        result = float2(sdBox2(localPos, boxExtents), 3.0f);
    }
    
    return result;
}

//-------------------------------------------------------------------------------------------------------------------

float2 Function(float3 Position)
{
    float2 roomDist = float2(room(Position), 1.0f);
	
	// Distance to the infinite gem grid
    float2 gems = MapTreasureHoard(Position);
    
    // Union the gems with the room
    return opU(roomDist, gems);
}

bool IntersectBox(in Ray ray, in float3 minimum, in float3 maximum, out float timeIn, out float timeOut)
{
	float3 OMIN = (minimum - ray.o) / ray.d;
	float3 OMAX = (maximum - ray.o) / ray.d;
	float3 MAX = max(OMAX, OMIN);
	float3 MIN = min(OMAX, OMIN);
	timeOut = min(MAX.x, min(MAX.y, MAX.z));
	timeIn = max(max(MIN.x, 0.0), max(MIN.y, MIN.z));

	return timeOut > timeIn;
}

bool RayMarchingInsideCube(in Ray ray, in float start, in float final, out float2 val)
{
    val = float2(0.0f, -1.0f);
	
	float step = (final - start) / float(INTERVALS);
	float time = start;
	float3 Position = ray.o + time * ray.d;
    float2 right;
    float2 left = Function(Position);
	
	for (int i = 0; i < INTERVALS; i++)
	{
		time += step;
		Position += step * ray.d;
		right = Function(Position);
		if (left.x * right.x < 0.0f)
		{
            val = float2(time + right.x * step / (left.x - right.x), right.y);
			return true;
		}
		left = right;
	}

	return false;
}

float3 CalcNormal(float3 Position) {
	float A = Function(Position + AxisX * STEP).x
		- Function(Position - AxisX * STEP).x;
	float B = Function(Position + AxisY * STEP).x
		- Function(Position - AxisY * STEP).x;
	float C = Function(Position + AxisZ * STEP).x
		- Function(Position - AxisZ * STEP).x;
	return normalize(float3 (A, B, C));
}

float4 Phong(float3 n, float3 l, float3 v, float shininess, float4 diffuseColor, float4 specularColor)
{
	float NdotL = dot(n, l);
	float diff = saturate(NdotL);
	float3 r = reflect(l, n);
	float spec = pow(saturate(dot(v, r)), shininess) * (NdotL > 0.0);
	return diff * diffuseColor + spec * specularColor;
}

float4 Shade(float3 Position, float3 normal, float3 viewDir, float3 color)
{
	float4 diff = float4(color, 1.0f);
	float4 spec = diff * 0.15f;
	
	//Lights are above the ceiling, so a base albedo to show the ceiling
   // float4 output = float4(color, 1.0) * 0.4f;
	float4 output = (float4)0;
	
	float3 lightDir;

	for (int i = 0; i < NUMLIGHTS; i++)
	{
		lightDir = normalize(LightPos[i].xyz - Position);
		output += float4(color, 1.0) * Phong(normal, lightDir, viewDir, 40.0f, diff, spec);
	}

	return saturate(LightColor * output);
}

float2 CalcUV(float3 Position, float3 normal)
{
    float3 absN = abs(normal);
	
    if (absN.x > absN.y && absN.x > absN.z)
        return Position.zy;
	
    if (absN.y > absN.x && absN.y > absN.z)
        return Position.xz;

	return Position.xy;

}

float4 RayMarching(Ray ray)
{
	float4 result = (float4)0;
	float start, final;
	float2 t;
	
	if (IntersectBox(ray, BoxMinimum, BoxMaximum, start, final))
	{
		if (RayMarchingInsideCube(ray, start, final, t))
		{
			float3 Position = ray.o + ray.d * t.x;
			float3 normal = CalcNormal(Position);
			
            //return float4(normal, 1.0f);
			
            float2 UV = CalcUV(Position, normal);
			
            float3 color = (float3) 0;
			
            int matID = (int) (t.y + 0.5f);
            if (matID == 1)
            {
                color = txTexture.SampleLevel(txSampler, 0.5f * UV, 0).rgb;
            }
            else if (matID == 2)
            {
                color = float3(0.8f, 0.9f, 1.0f);
            }
            else if (matID == 3)
            {
                color = float3(0.8f, 0.1f, 0.2f);
            }

			result = Shade(Position, normal, -ray.d, color);
		}
	}
	return result;

}

float4 main(VS_Canvas input) : SV_TARGET
{
	float zoom = 0.004;
	float2 xy = zoom * input.canvasXY;
	float distEye2Canvas = nearPlane;
	float3 PixelPos = float3(xy, distEye2Canvas);
	
	Ray eyeRay;
	eyeRay.o = Eye.xyz;
	eyeRay.d = normalize(PixelPos - Eye.xyz);	//view direction
	
	return RayMarching(eyeRay);
}
