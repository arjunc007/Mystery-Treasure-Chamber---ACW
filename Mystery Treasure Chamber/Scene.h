#pragma once
#include "SceneObject.h"
#include "Content\ShaderStructures.h"
#include <vector>
#include <memory>
#include <wrl/client.h>

class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	bool Initialize(ID3D11Device* device);

	void AddMeshObject(std::shared_ptr<SceneObject> obj);
	void AddBackgroundObject(std::shared_ptr<SceneObject> obj);

	void Update(float deltaTime);
	void Render(ID3D11DeviceContext* context);

	Mystery_Treasure_Chamber::ModelViewProjectionConstantBuffer MatrixData;
	Mystery_Treasure_Chamber::PixelShaderConstantBuffer LightingData;
	Mystery_Treasure_Chamber::ConstantBuffer TimeData;

private:
	std::vector<std::shared_ptr<SceneObject>> m_backgroundObjects;
	std::vector<std::shared_ptr<SceneObject>> m_meshObjects;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_mvpConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_psConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_timeConstantBuffer;
};

