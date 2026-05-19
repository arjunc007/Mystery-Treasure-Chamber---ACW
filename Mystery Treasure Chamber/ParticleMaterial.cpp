#include "pch.h"
#include "Common\DDSTextureLoader.h"
#include "Common\DirectXHelper.h"
#include "ParticleMaterial.h"

#include <fstream>

using namespace Mystery_Treasure_Chamber;

HRESULT ParticleMaterial::ReadShaderFile(const std::wstring& filename, std::vector<char>& shaderData)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) return E_FAIL;
	size_t fileSize = (size_t)file.tellg();
	shaderData.resize(fileSize);
	file.seekg(0);
	file.read(shaderData.data(), fileSize);
	file.close();
	return S_OK;
}

bool ParticleMaterial::LoadUpdateShaders(ID3D11Device* device, const std::wstring& vsFilename, const std::wstring& gsFilename)
{
	std::vector<char> vsData, gsData;

	if (FAILED(ReadShaderFile(vsFilename, vsData))) return false;
	device->CreateVertexShader(vsData.data(), vsData.size(), nullptr, m_updateVS.GetAddressOf());

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Speed
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Size
		{ "TEXCOORD", 3, DXGI_FORMAT_R32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Age
		{ "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,        0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }  // Type
	};
	device->CreateInputLayout(layout, ARRAYSIZE(layout), vsData.data(), vsData.size(), m_inputLayout.GetAddressOf());

	if (FAILED(ReadShaderFile(gsFilename, gsData))) return false;

	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// Stream, Semantic, Index, StartComp, CompCount, OutputSlot
		{ 0, "TEXCOORD", 0, 0, 3, 0 },
		{ 0, "TEXCOORD", 1, 0, 3, 0 }, // speed
		{ 0, "TEXCOORD", 2, 0, 2, 0 }, // size
		{ 0, "TEXCOORD", 3, 0, 1, 0 }, // age
		{ 0, "TEXCOORD", 4, 0, 1, 0 }  // type
	};

	UINT stride = 40; // Total size of a Particle (3*4 + 3*4 + 2*4 + 1*4 + 1*4 = 40 bytes)

	HRESULT hr = device->CreateGeometryShaderWithStreamOutput(
		gsData.data(), gsData.size(),
		pDecl, ARRAYSIZE(pDecl),
		&stride, 1, D3D11_SO_NO_RASTERIZED_STREAM,
		nullptr, m_updateGS.GetAddressOf()
	);

	return SUCCEEDED(hr);
}

bool ParticleMaterial::LoadRenderShaders(ID3D11Device* device, const std::wstring& vsFilename, const std::wstring& gsFilename, const std::wstring& psFilename)
{
	std::vector<char> vsData, gsData, psData;

	if (SUCCEEDED(ReadShaderFile(vsFilename, vsData)))
		device->CreateVertexShader(vsData.data(), vsData.size(), nullptr, m_renderVS.GetAddressOf());

	if (SUCCEEDED(ReadShaderFile(gsFilename, gsData)))
		device->CreateGeometryShader(gsData.data(), gsData.size(), nullptr, m_renderGS.GetAddressOf());

	if (SUCCEEDED(ReadShaderFile(psFilename, psData)))
		device->CreatePixelShader(psData.data(), psData.size(), nullptr, m_renderPS.GetAddressOf());

	return m_renderVS && m_renderGS && m_renderPS;
}

bool ParticleMaterial::LoadTextures(ID3D11Device* device, const std::wstring& fireTexFilename, const std::wstring& noiseTexFilename)
{
	if (!fireTexFilename.empty())
	{
		DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, fireTexFilename.c_str(), nullptr, m_fireTexture.GetAddressOf()));
	}
	if (!fireTexFilename.empty())
	{
		DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, noiseTexFilename.c_str(), nullptr, m_noiseTexture.GetAddressOf()));
	}

	return true;
}

bool ParticleMaterial::InitializeStates(ID3D11Device* device)
{
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Smooth filtering
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;    // Wrap is essential for the noise texture!
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	if (FAILED(device->CreateSamplerState(&sampDesc, m_sampler.GetAddressOf()))) return false;

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(device->CreateBlendState(&blendDesc, m_additiveBlendState.GetAddressOf()))) return false;

	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;

	if (FAILED(device->CreateDepthStencilState(&depthDesc, m_noDepthWriteState.GetAddressOf()))) return false;

	return true;
}

void ParticleMaterial::BindForUpdate(ID3D11DeviceContext* context, ID3D11Buffer* timeBuffer)
{
	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader(m_updateVS.Get(), nullptr, 0);
	context->GSSetShader(m_updateGS.Get(), nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	context->GSSetConstantBuffers(0, 1, &timeBuffer);

	context->GSSetShaderResources(0, 1, m_noiseTexture.GetAddressOf());
	context->GSSetSamplers(0, 1, m_sampler.GetAddressOf());
}

void ParticleMaterial::BindForRender(ID3D11DeviceContext* context, ID3D11Buffer* matrixBuffer, ID3D11Buffer* resizeBuffer, ID3D11Buffer* viewBuffer)
{
	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader(m_renderVS.Get(), nullptr, 0);
	context->GSSetShader(m_renderGS.Get(), nullptr, 0);
	context->PSSetShader(m_renderPS.Get(), nullptr, 0);

	ID3D11Buffer* gsBuffers[] = { matrixBuffer, resizeBuffer, viewBuffer };
	context->GSSetConstantBuffers(0, 3, gsBuffers);

	context->PSSetShaderResources(0, 1, m_fireTexture.GetAddressOf());
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(m_additiveBlendState.Get(), blendFactor, 0xffffffff);
	context->OMSetDepthStencilState(m_noDepthWriteState.Get(), 0);
}

void ParticleMaterial::Unbind(ID3D11DeviceContext* context)
{
	context->GSSetShader(nullptr, nullptr, 0);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	context->OMSetDepthStencilState(nullptr, 0);
}