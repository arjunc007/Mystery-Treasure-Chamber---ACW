//Simply pass particle data to geometry shader
struct VS_INPUT {
	float3 position : POSITION;
	float3 speed : TEXCOORD0;
	float2 size : TEXCOORD1;
	float age : TEXCOORD2;
	uint type : TEXCOORD3;
};

VS_INPUT main( VS_INPUT input )
{
	return input;
}