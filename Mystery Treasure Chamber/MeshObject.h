#pragma once
#include "SceneObject.h"
#include "Material.h"
#include <vector>

class MeshObject : public SceneObject
{
public:
	MeshObject(ID3D11Device* device,
		const void* vertexData, UINT vertexSize, UINT vertexCount,
		const std::vector<unsigned short>& indices,
		std::shared_ptr<Material> material);

	void Draw(ID3D11DeviceContext* context) override;

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
	UINT m_indexCount;
	UINT m_vertexStride;

	std::shared_ptr<Material> m_material;
};

