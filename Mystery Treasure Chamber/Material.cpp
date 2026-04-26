#include "Material.h"
#include "Common\DDSTextureLoader.h"
#include "Common\DirectXHelper.h"
#include <fstream>

std::vector<char> Material::ReadCompiledShader(const std::wstring& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		return {};
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

bool Material::Initialize(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath, const std::wstring& texturePath)
{
	auto vsData = ReadCompiledShader(vsPath);
	if (vsData.empty())
	{
		return false;
	}

	device->CreateVertexShader(vsData.data(), vsData.size(), nullptr, m_vertexShader.GetAddressOf());

	static const D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
	device->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), vsData.data(), vsData.size(), m_inputLayout.GetAddressOf());

	auto psData = ReadCompiledShader(psPath);
	if (psData.empty())
	{
		return false;
	}

	device->CreatePixelShader(psData.data(), psData.size(), nullptr, m_pixelShader.GetAddressOf());

	DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, texturePath.c_str(), nullptr, m_texture.GetAddressOf()));

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&samplerDesc, m_samplerState.GetAddressOf());

	return true;
}

void Material::Bind(ID3D11DeviceContext* context)
{
	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
}