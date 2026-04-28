#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <vector>

class Material
{
public:
	Material() = default;
	~Material() = default;

	bool Initialize(ID3D11Device* device,
		const std::wstring& vertexShaderPath,
		const std::wstring& pixelShaderPath,
		const std::wstring& diffusePath,
		const std::wstring& normalPath = L"",
		const std::wstring& hullShaderPath = L"",
		const std::wstring& domainShaderPath = L"",
		const std::wstring& displacementPath = L""
	);

	void Bind(ID3D11DeviceContext* context);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11HullShader> m_hullShader;
	Microsoft::WRL::ComPtr<ID3D11DomainShader> m_domainShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_displacementTexture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
	
	std::vector<char> ReadCompiledShader(const std::wstring& filename);
};

