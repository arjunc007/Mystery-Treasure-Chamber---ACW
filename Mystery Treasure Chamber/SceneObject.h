#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>

//Base class for all objects that can exist in the scene
class SceneObject
{
public:
	virtual ~SceneObject() = default;

	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };

	virtual void Draw(ID3D11DeviceContext* context) = 0;

	DirectX::XMMATRIX GetModelMatrix() const
	{
		using namespace DirectX;
		XMMATRIX translation = XMMatrixTranslation(Position.x, Position.y, Position.z);
		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
		XMMATRIX scaling = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

		return scaling * rotation * translation;
	}
};

