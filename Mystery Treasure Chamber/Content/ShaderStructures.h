#pragma once

namespace Mystery_Treasure_Chamber
{
	struct ConstantBuffer
	{
		float time;
		float deltaTime;
		DirectX::XMFLOAT2 padding;
	};

	struct ChangesOnResizeConstantBuffer
	{
		float height;
		float width;
		DirectX::XMFLOAT2 padding;
	};

	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	struct VertexPositionTextureNTB
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 texture;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT3 binormal;
	};

	struct PixelShaderConstantBuffer
	{
		DirectX::XMFLOAT4 eye;
		DirectX::XMFLOAT4 lightColor;
		DirectX::XMFLOAT4 backgroundColor;
		DirectX::XMFLOAT4 lightPos[3];
		float nearPlane;
		float farPlane;
		DirectX::XMFLOAT2 padding;
	};

	struct Particle {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 speed;
		DirectX::XMFLOAT2 size;
		float age;
		int	type;
	};
}