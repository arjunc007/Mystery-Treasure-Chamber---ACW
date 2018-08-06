//Mesh pixel shader which takes one texture for the model

SamplerState txSampler : register(s0);
Texture2D txTexture : register(t0);

#define NUMLIGHTS 3

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

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 Texture : TEXCOORD0;
	float3 normal : NORMAL;
};

float4 Phong(float3 n, float3 l, float3 v, float shininess, float4 diffuseColor, float4 specularColor)
{
	float NdotL = dot(n, l);
	float diff = saturate(NdotL);
	float3 r = reflect(l, n);
	float spec = pow(saturate(dot(v, r)), shininess) * (NdotL > 0.0);
	return diff * diffuseColor + spec * specularColor;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 spec = float4(1, 1, 1, 1);
	float4 diff = txTexture.Sample(txSampler, Input.Texture);

	float4 color = (float4)0;
	float3 lightDir;
	float3 viewDir = normalize(Eye.xyz - Input.Position.xyz);

	for (int i = 0; i < 1; i++)
	{
		lightDir = normalize(LightPos[i].xyz - Input.Position.xyz);
		color += Phong(Input.normal, lightDir, viewDir, 40, diff, spec);
	}

	return saturate(LightColor * color);

}



