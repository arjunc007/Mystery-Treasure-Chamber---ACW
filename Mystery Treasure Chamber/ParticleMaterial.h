#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <vector>

namespace Mystery_Treasure_Chamber
{
	class ParticleMaterial
	{
	public:
		ParticleMaterial() = default;
		~ParticleMaterial() = default;

		bool LoadUpdateShaders(ID3D11Device* device, const std::wstring& vsFilename, const std::wstring& gsFilename);
		bool LoadRenderShaders(ID3D11Device* device, const std::wstring& vsFilename, const std::wstring& gsFilename, const std::wstring& psFilename);
		bool LoadTextures(ID3D11Device* device, const std::wstring& fireTexFilename, const std::wstring& noiseTexFilename);
		
		bool InitializeStates(ID3D11Device* device);

		void BindForUpdate(ID3D11DeviceContext* context);
		void BindForRender(ID3D11DeviceContext* context);
		void Unbind(ID3D11DeviceContext* context);

	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_updateVS;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_updateGS;

		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_renderVS;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_renderGS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_renderPS;

		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fireTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_noiseTexture;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;

		Microsoft::WRL::ComPtr<ID3D11BlendState> m_additiveBlendState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_noDepthWriteState;

		HRESULT ReadShaderFile(const std::wstring& filename, std::vector<char>& shaderData);
	};
}