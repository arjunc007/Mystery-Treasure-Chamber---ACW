#pragma once
#include "Content/ShaderStructures.h"
#include "ParticleMaterial.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

namespace Mystery_Treasure_Chamber
{
	class ParticleSystem
	{
	public:
		ParticleSystem();
	
		bool Initialize(ID3D11Device* device, const std::vector<DirectX::XMFLOAT3>& emitterPositions);
	
		void UpdateAndRender(ID3D11DeviceContext* context,
			ID3D11Buffer* timeBuffer,
			ID3D11Buffer* matrixBuffer,
			ID3D11Buffer* resizeBuffer,
			ID3D11Buffer* viewBuffer);
	
		void SetMaterial(std::shared_ptr<ParticleMaterial> material)
		{
			m_material = material;
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleBuffers[2];
	
		std::shared_ptr<ParticleMaterial> m_material;
		bool m_firstRun;
		uint32_t m_initialEmittersCount;
		uint32_t m_pingPongToggle;
		uint32_t m_maxParticles;
	
		bool CreateParticleBuffer(ID3D11Device* device, ID3D11Buffer** buffer, const std::vector<Particle>* initialData = nullptr);
	};
}