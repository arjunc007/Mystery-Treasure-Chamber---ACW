//The second raymarching pass, which utilises the previous one as texture input

Texture2D txRoom : register(t0);
Texture2D txTexture : register(t1);
Texture2D txNormal : register(t2);
SamplerState txSampler : register(s0);

#define NUMLIGHTS 3

//static float4 Eye = float4(0, 3.5, 5, 1);//eye position
//static float nearPlane = 1.0;
//static float farPlane = 100.0;
//static float4 LightColor = float4(1, 1, 1, 1);
//static float4 backgroundColor = float4(0.1, 0.2, 0.3, 1);
//static float3 LightPos[NUMLIGHTS] = { float3(-50, 10, -5), float3(60, 10, 5), float3(0, -10, 0) };

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
	float2 tex : TEXCOORD1;
};

//------------------------------------------------------------------------------------------------------------------

float sdSphere(float3 p, float s)
{
	return length(p) - s;
}

float udBox(float3 p, float3 b)
{
	return length(max(abs(p) - b, 0.0));
}

float udRoundBox(float3 p, float3 b, float r)
{
	return length(max(abs(p) - b, 0.0)) - r;
}

float sdBox(float3 p, float3 b)
{
	float3 d = abs(p) - b;
	return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdTorus(float3 p, float2 t)
{
	float2 q = float2(length(p.xz) - t.x, p.y);
	return length(q) - t.y;
}

float sdCylinder(float3 p, float3 c)
{
	return length(p.xz - c.xy) - c.z;
}

float sdCone(float3 p, float2 c)
{
	// c must be normalized
	float q = length(p.xy);
	return dot(c, float2(q, p.z));
}

float sdCone(in float3 p, in float3 c)
{
	float2 q = float2(length(p.xz), p.y);
	float d1 = -q.y - c.z;
	float d2 = max(dot(q, c.xy), q.y);
	return length(max(float2(d1, d2), 0.0)) + min(max(d1, d2), 0.);
}

float sdPlane(float3 p, float4 n)
{
	// n must be normalized
	return dot(p, n.xyz) + n.w;
}

float sdCapsule(float3 p, float3 a, float3 b, float r)
{
	float3 pa = p - a, ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
	return length(pa - ba * h) - r;
}

float sdCappedCylinder(float3 p, float2 h)
{
	float2 d = abs(float2(length(p.xz), p.y)) - h;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float sdCappedCone(in float3 p, in float3 c)
{
	float2 q = float2(length(p.xz), p.y);
	float2 v = float2(c.z*c.y / c.x, -c.z);
	float2 w = v - q;
	float2 vv = float2(dot(v, v), v.x*v.x);
	float2 qv = float2(dot(v, w), v.x*w.x);
	float2 d = max(qv, 0.0)*qv / vv;
	return sqrt(dot(w, w) - max(d.x, d.y)) * sign(max(q.y*v.x - q.x*v.y, w.y));
}



//------------------------------------------------------------------------------------------------------------------

float softAbs2(float x, float a)
{
	float xx = 2.0*x / a;
	float abs2 = abs(xx);
	if (abs2<2.0)
		abs2 = 0.5*xx*xx*(1.0 - abs2 / 6) + 2.0 / 3.0;
	return abs2 * a / 2.0;
}

float softMax2(float x, float y, float a)
{
	return 0.5*(x + y + softAbs2(x - y, a));
}

float softMin2(float x, float y, float a)
{
	return   -0.5*(-x - y + softAbs2(x - y, a));
}

float Union(float d1, float d2)
{
	return min(d1, d2);
}

float Subtract(float d1, float d2)
{
	return max(-d2, d1);
}

float3 Repeat(float3 p, float3 c)
{
	return fmod(p, c) - 0.5*c;
}

float3 Twist(float3 p)
{
	float  c = cos(10.0*p.y + 10.0);
	float  s = sin(10.0*p.y + 10.0);
	float2x2   m = float2x2(c, -s, s, c);
	return float3(mul(m, p.xz), p.y);
}

float room(float3 Position)
{
	return -sdBox(Position, float3(5, 5, 5));
}

float pillars(float3 Position)
{
	float Fun = sdCylinder(Position + float3(3.5, 0, 3.5), float3(0, 0, 0.5));
	Fun = min(Fun, sdCylinder(Position + float3(-3.5, 0, 3.5), float3(0, 0, 0.5)));
	Fun = min(Fun, sdCylinder(Position + float3(-3.5, 0, 0), float3(0, 0, 0.5)));
	Fun = min(Fun, sdCylinder(Position + float3(3.5, 0, 0), float3(0, 0, 0.5)));

	return Fun;
}
//-------------------------------------------------------------------------------------------------------------------

float Function(float3 Position)
{
	float Fun = pillars(Position);

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
	float4 spec = float4(1, 1, 1, 1);

	float4 output = (float4)0;
	float3 lightDir;

	for (int i = 0; i < NUMLIGHTS; i++)
	{
		lightDir = normalize(LightPos[i] - Position);
		output += Phong(normal, lightDir, viewDir, 40, float4(color, 1.0f), spec);
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

float4 RayMarching(Ray ray, float2 tex)
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
			
			float P = Function(Position);

			float2 UV = CalcUV(Position, normal);

			float3 color = txTexture.Sample(txSampler, 0.5 * UV);

			float3 texNormal = normalize(2 * txNormal.Sample(txSampler, 0.5 * UV).rgb - float3(1, 1, 1));

			result = Shade(Position, texNormal, ray.d, color);
		}
		else
		{
			result = txRoom.Sample(txSampler, tex);
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

return RayMarching(eyeRay, input.tex);
}
