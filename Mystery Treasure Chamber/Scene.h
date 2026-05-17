#pragma once
#include "SceneObject.h"
#include "Common/StepTimer.h"
#include "Content\ShaderStructures.h"
#include <vector>
#include <memory>
#include <wrl/client.h>

namespace Mystery_Treasure_Chamber
{
	class ParticleSystem;
}

class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	bool Initialize(ID3D11Device* device);
	bool CreateRoomRenderTarget(ID3D11Device* device, UINT width, UINT height);

	void AddMeshObject(std::shared_ptr<SceneObject> obj);
	void AddBackgroundObject(std::shared_ptr<SceneObject> obj);

	void Update(const DX::StepTimer& timer);
	void Render(ID3D11DeviceContext* context);

	Mystery_Treasure_Chamber::ModelViewProjectionConstantBuffer MatrixData;
	Mystery_Treasure_Chamber::PixelShaderConstantBuffer LightingData;
	Mystery_Treasure_Chamber::ChangesOnResizeConstantBuffer ScreenSizeData;
	Mystery_Treasure_Chamber::ConstantBuffer TimeData;

private:
	void SetupSceneObjects();

	std::vector<std::shared_ptr<SceneObject>> m_backgroundObjects;
	std::vector<std::shared_ptr<SceneObject>> m_meshObjects;
	std::shared_ptr<SceneObject> m_floor;
	std::shared_ptr<Mystery_Treasure_Chamber::ParticleSystem> m_fireSystem;

	Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_mvpConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_psConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_sizeConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_timeConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_defaultDepthState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_depthWriteOffState;
};

