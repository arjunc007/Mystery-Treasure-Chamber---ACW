#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

namespace Mystery_Treasure_Chamber
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();


	private:
		void Rotate(float radians);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		m_modelInputLayout;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		m_particleInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_cubeVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_quadVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_snakeVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_particleVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_particleVertexBufferSO;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_canvasVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_roomPixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_pillarPixelShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_modelVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_modelPixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_floorPixelShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_particleVertexShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_particleVertexShaderSO;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	m_particleGeometryShader;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	m_particleGeometryShaderSO;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_particlePixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_timeBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_changesOnResizeConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_psConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11BlendState>		m_additiveBlend;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_noWriteDepthState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_DisableCullState;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_groundVertexShader;
		Microsoft::WRL::ComPtr<ID3D11HullShader>		m_hullShader;
		Microsoft::WRL::ComPtr<ID3D11DomainShader>		m_domainShader;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_wireframeState;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>			m_renderTargetTexture;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_renderTargetView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>		m_samplerState;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scalesTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_floorTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_floorDisplacementTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_floorNormalTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pillarTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pillarNormalTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pillarHeightTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_wallTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_wallHeightTexture;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_cullFrontState;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fireTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_noiseTexture;

		// System resources for cube geometry.
		ConstantBuffer						m_timeBufferData;
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		PixelShaderConstantBuffer			m_psConstantBufferData;
		ChangesOnResizeConstantBuffer		m_changesOnResizeConstantBufferData;
		uint32	m_indexCount;
		uint32	m_vertexCount;
		uint32 m_maxParticles;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
	};
}

