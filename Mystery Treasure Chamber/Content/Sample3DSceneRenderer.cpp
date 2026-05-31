#include "pch.h"
#include "Sample3DSceneRenderer.h"
#include "Common\DirectXHelper.h"
#include "Common\DDSTextureLoader.h"
#include "Scene.h"
//#include "..\Common\BasicShapes.h"

#include <fstream>

using namespace Mystery_Treasure_Chamber;

using namespace DirectX;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_deviceResources(deviceResources)
{
	m_scene = std::make_unique<Scene>();

	m_scene->Initialize(m_deviceResources->GetWindowHandle(), m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

Sample3DSceneRenderer::~Sample3DSceneRenderer() = default;

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	m_scene->CreateRoomRenderTarget(m_deviceResources->GetD3DDevice(), m_deviceResources->GetOutputWidth(), m_deviceResources->GetOutputHeight());
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{	
	m_scene->Update(timer);
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset render targets to the screen.

	m_scene->Render(context);
	

	//Draw the particles using geometry shader
	//----------------------------------------------------------------------------------------------------------------------------------------------
	/*
	// Each vertex is one instance of the VertexPositionColor struct.
	const UINT stride = sizeof(Particle);
	const UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_particleVertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	context->IASetInputLayout(m_particleInputLayout.Get());

	context->PSSetShader(nullptr, nullptr, 0);

	// Attach our vertex shader.
	context->VSSetShader(
		m_particleVertexShaderSO.Get(),
		nullptr,
		0
	);

	//Attach the geometry shader
	context->GSSetShader(
		m_particleGeometryShaderSO.Get(),
		nullptr,
		0
	);

	context->GSSetConstantBuffers(0, 1, m_timeBuffer.GetAddressOf());

	context->GSSetShaderResources(0, 1, m_noiseTexture.GetAddressOf());
	context->GSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	context->SOSetTargets(1, m_particleVertexBufferSO.GetAddressOf(), &offset);

	context->DrawAuto();

	//Done streaming out
	ID3D11Buffer* bufferArray[1] = { 0 };
	context->SOSetTargets(1, bufferArray, &offset);

	std::swap(m_particleVertexBuffer, m_particleVertexBufferSO);

	// Attach our vertex shader.
	context->VSSetShader(
		m_particleVertexShader.Get(),
		nullptr,
		0
	);

	XMMATRIX model = XMMatrixIdentity() * XMMatrixScaling(3, 3, 3);

	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(model));

	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
	);

	// Send the constant buffer to the graphics device.
	context->GSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->GSSetConstantBuffers1(
		1,
		1,
		m_changesOnResizeConstantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->GSSetConstantBuffers1(
		2,
		1,
		m_psConstantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	//Attach the geometry shader
	context->GSSetShader(
		m_particleGeometryShader.Get(),
		nullptr,
		0
	);

	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
	context->PSSetShaderResources(0, 1, m_fireTexture.GetAddressOf());

	// Attach our pixel shader.
	context->PSSetShader(
		m_particlePixelShader.Get(),
		nullptr,
		0
	);

	//Enable blending
	context->OMSetBlendState(m_additiveBlend.Get(), nullptr, 0xFFFFFF);

	//Disable depth buffer writes
	//context->OMSetDepthStencilState(m_noWriteDepthState.Get(), 1);

	//context->RSSetState(m_DisableCullState.Get());

	// Draw the objects.
	context->DrawAuto();

	//disable blending
	context->OMSetBlendState(nullptr, nullptr, 0xFFFFFF);

	//Reset depth stencil state
	//context->OMSetDepthStencilState(nullptr, 0);

	//context->RSSetState(nullptr);
	//Unset geometry shaders
	context->GSSetShader(
		nullptr,
		nullptr,
		0
	);*/
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	auto d3dDevice = m_deviceResources->GetD3DDevice();

	//Particle Shaders
	/*
	{
		auto vsData = DX::ReadData(L"ParticleVertexShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &m_particleVertexShader));

		auto vsSOData = DX::ReadData(L"ParticleVertexShaderSO.cso");
		DX::ThrowIfFailed(d3dDevice->CreateVertexShader(vsSOData.data(), vsSOData.size(), nullptr, &m_particleVertexShaderSO));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 3, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		DX::ThrowIfFailed(d3dDevice->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), vsSOData.data(), vsSOData.size(), &m_particleInputLayout));

		auto psData = DX::ReadData(L"ParticlePixelShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &m_particlePixelShader));

		auto gsData = DX::ReadData(L"GeometryShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreateGeometryShader(gsData.data(), gsData.size(), nullptr, &m_particleGeometryShader));

		auto gsSOData = DX::ReadData(L"GeometryShaderSO.cso");
		D3D11_SO_DECLARATION_ENTRY pDecl[] = {
			{ 0, "POSITION", 0, 0, 3, 0 },
			{ 0, "TEXCOORD", 0, 0, 3, 0 },
			{ 0, "TEXCOORD", 1, 0, 2, 0 },
			{ 0, "TEXCOORD", 2, 0, 1, 0 },
			{ 0, "TEXCOORD", 3, 0, 1, 0 },
		};
		DX::ThrowIfFailed(d3dDevice->CreateGeometryShaderWithStreamOutput(gsSOData.data(), gsSOData.size(), pDecl, ARRAYSIZE(pDecl), NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, &m_particleGeometryShaderSO));
	}*/

	//Blending and render states
	{
		D3D11_BLEND_DESC additiveBlendDesc;
		ZeroMemory(&additiveBlendDesc, sizeof(D3D11_BLEND_DESC));
		additiveBlendDesc.AlphaToCoverageEnable = FALSE;
		additiveBlendDesc.RenderTarget[0].BlendEnable = TRUE;
		additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;
		DX::ThrowIfFailed(d3dDevice->CreateBlendState(&additiveBlendDesc, &m_additiveBlend));

		D3D11_RASTERIZER_DESC cullDesc;
		cullDesc.FillMode = D3D11_FILL_SOLID;
		cullDesc.CullMode = D3D11_CULL_BACK;
		DX::ThrowIfFailed(d3dDevice->CreateRasterizerState(&cullDesc, &m_DisableCullState));
	}

	//Particles
	/*
	{
		m_maxParticles = 1000;

		// Create a safe block of memory for all 1000 particles, initialized to zero
		std::vector<Particle> particleVertices(m_maxParticles);

		// Set the very first particle (the emitter)
		particleVertices[0] = { XMFLOAT3(0,0,0), XMFLOAT3(0,0,0), XMFLOAT2(0.015f, 0.015f), 0.0f, 0 };

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = particleVertices.data();

		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(Particle) * m_maxParticles, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT);

		// Create the buffers safely
		DX::ThrowIfFailed(d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_particleVertexBuffer));

		// For the Stream Output buffer, we don't pass initial data (pass nullptr)
		DX::ThrowIfFailed(d3dDevice->CreateBuffer(&vertexBufferDesc, nullptr, &m_particleVertexBufferSO));
	}
	*/

	m_loadingComplete = true;
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_canvasVertexShader.Reset();
	m_particleVertexShader.Reset();
	m_particleVertexShaderSO.Reset();
	m_inputLayout.Reset();
	m_modelInputLayout.Reset();
	m_particleInputLayout.Reset();
	m_roomPixelShader.Reset();
	m_pillarPixelShader.Reset();
	m_particlePixelShader.Reset();
	m_particleGeometryShader.Reset();
	m_constantBuffer.Reset();
	m_psConstantBuffer.Reset();
	m_changesOnResizeConstantBuffer.Reset();
	m_cubeVertexBuffer.Reset();
	m_particleVertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_additiveBlend.Reset();
	m_noWriteDepthState.Reset();
	m_wireframeState.Reset();
	m_renderTargetTexture.Reset();
	m_renderTargetView.Reset();
	m_shaderResourceView.Reset();
	m_samplerState.Reset();
	m_cullFrontState.Reset();
	m_wallHeightTexture.Reset();
	m_wallTexture.Reset();
	m_fireTexture.Reset();
	m_noiseTexture.Reset();
	m_particleVertexBufferSO.Reset();
}