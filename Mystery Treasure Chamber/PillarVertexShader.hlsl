// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

cbuffer ChangesOnResizeConstantBuffer : register(b1)
{
    float height;
    float width;
    float2 padding;
}

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
    float3 pos : POSITION;
    float3 color : COLOR0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION; // Screen position (used by rasterizer)
    float3 WorldPos : POSITION; // 3D position in the world
    float2 tex : TEXCOORD1;
};

PS_INPUT main(VertexShaderInput input)
{
    PS_INPUT Output;
    
    float4 worldPosition = mul(float4(input.pos, 1.0f), model);
    Output.WorldPos = worldPosition.xyz;
    
    float4 viewPosition = mul(worldPosition, view);
    
    Output.Position = mul(viewPosition, projection);
    
    Output.tex = float2((Output.Position.x + 1) / 2, (Output.Position.y - 1) / -2);

    return Output;
}