//This is for the room raymarching or the first raymarching pass

Texture2D txTexture : register(t0);
SamplerState txSampler : register(s0);

#define NUMLIGHTS 3

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
float4 LightPos[3];
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

float room(float3 Position)
{
	return  -sdBox(Position, float3(5, 5, 5));
}

//-------------------------------------------------------------------------------------------------------------------

float Function(float3 Position)
{
	float Fun = room(Position);
	
	return Fun;
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

bool RayMarchingInsideCube(in Ray ray, in float start, in float final, out float val)
{
	val = 0.0;
	float step = (final - start) / float(INTERVALS);
	float time = start;
	float3 Position = ray.o + time * ray.d;
	float right, left = Function(Position);
	
	for (int i = 0; i < INTERVALS; i++)
	{
		time += step;
		Position += step * ray.d;
		right = Function(Position);
		if (left * right < 0.0)
		{
			val = time + right * step / (left - right);
			return true;
		}
		left = right;
	}

	return false;
}

float3 CalcNormal(float3 Position) {
	float A = Function(Position + AxisX * STEP)
		- Function(Position - AxisX * STEP);
	float B = Function(Position + AxisY * STEP)
		- Function(Position - AxisY * STEP);
	float C = Function(Position + AxisZ * STEP)
		- Function(Position - AxisZ * STEP);
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
	float4 diff = float4(1, 1, 1, 1);
	float4 spec = float4(0, 1, 0, 1);

	float4 output = (float4)0;
	float3 lightDir;

	for (int i = 0; i < NUMLIGHTS; i++)
	{
		lightDir = normalize(LightPos[i] - Position);
		output += float4(color, 1.0) * Phong(normal, lightDir, viewDir, 40, diff, spec);
	}

	return saturate(LightColor * output);
}

float2 CalcUV(float3 Position, float3 normal)
{
	float3 u = float3(normal.y, -normal.x, 0);
	u = normalize(u);
	float3 v = cross(normal, u);

	return float2(dot(u, Position), dot(v, Position));

}

float4 RayMarching(Ray ray)
{
	float4 result = (float4)0;
	float start, final;
	float t;
	if (IntersectBox(ray, BoxMinimum, BoxMaximum, start, final))
	{
		if (RayMarchingInsideCube(ray, start, final, t))
		{
			float3 Position = ray.o + ray.d * t;
			float3 normal = CalcNormal(Position);

			float3 color = txTexture.Sample(txSampler, CalcUV(Position, normal));

			result = Shade(Position, normal, ray.d, color);
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
