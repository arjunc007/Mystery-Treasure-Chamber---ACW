//Simply pass particle data to geometry shader
struct VS_INPUT {
    float3 position : TEXCOORD0;
	float3 speed : TEXCOORD1;
	float2 size : TEXCOORD2;
	float age : TEXCOORD3;
	uint type : TEXCOORD4;
};

VS_INPUT main( VS_INPUT input )
{
	return input;
}