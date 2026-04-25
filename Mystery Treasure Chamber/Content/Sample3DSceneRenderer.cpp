#include "pch.h"
#include "Sample3DSceneRenderer.h"
#include "Common\DirectXHelper.h"
#include "DDSTextureLoader.h"
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
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	float width = static_cast<float>(m_deviceResources->GetOutputWidth());
	float height = static_cast<float>(m_deviceResources->GetOutputHeight()); 
	float aspectRatio = width / height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	m_changesOnResizeConstantBufferData.height = height;
	m_changesOnResizeConstantBufferData.width = width;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
	);

	XMMATRIX orientationMatrix = XMMatrixIdentity();

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 3.5f, 5.0f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));

	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = m_deviceResources->GetOutputWidth();
	textureDesc.Height = m_deviceResources->GetOutputHeight();
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// Create the render target texture.
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateTexture2D(&textureDesc, NULL, &m_renderTargetTexture)
	);

	// Setup the description of the render target view.
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateRenderTargetView(m_renderTargetTexture.Get(), &renderTargetViewDesc, m_renderTargetView.GetAddressOf())
	);

	// Setup the description of the shader resource view.
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view.
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_renderTargetTexture.Get(), &shaderResourceViewDesc, &m_shaderResourceView)
	);
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	m_timeBufferData.time = timer.GetTotalSeconds();
	m_timeBufferData.deltaTime = timer.GetElapsedSeconds();
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

//First, draw the room using ray marching
//----------------------------------------------------------------------------------------------------------------------------------------------
	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
	);

	context->UpdateSubresource1(
		m_changesOnResizeConstantBuffer.Get(),
		0,
		NULL,
		&m_changesOnResizeConstantBufferData,
		0,
		0,
		0
	);

	context->UpdateSubresource1(
		m_timeBuffer.Get(),
		0,
		NULL,
		&m_timeBufferData,
		0,
		0,
		0
	);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_cubeVertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_canvasVertexShader.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->VSSetConstantBuffers1(
		1,
		1,
		m_changesOnResizeConstantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->VSSetConstantBuffers1(
		2,
		1,
		m_timeBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_roomPixelShader.Get(),
		nullptr,
		0
	);

	context->UpdateSubresource1(
		m_psConstantBuffer.Get(),
		0,
		NULL,
		&m_psConstantBufferData,
		0,
		0,
		0
	);

	context->PSSetConstantBuffers1(
		0,
		1,
		m_psConstantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->PSSetShaderResources(0, 1, m_wallTexture.GetAddressOf());
	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_deviceResources->GetDepthStencilView());
	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);

//Second, draw the tessellated floor after the room walls.
//----------------------------------------------------------------------------------------------------------------------------------------------
	//Clear depth buffer
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	stride = sizeof(VertexPositionTextureNTB);
	offset = 0;

	context->IASetVertexBuffers(
		0,
		1,
		m_quadVertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	context->IASetInputLayout(m_modelInputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_groundVertexShader.Get(),
		nullptr,
		0
	);

	context->HSSetShader(
		m_hullShader.Get(),
		nullptr,
		0
	);

	XMMATRIX model = XMMatrixIdentity() * XMMatrixScaling(5, 5, 5) * XMMatrixTranslation(0.0f, -2.5f, 0.0f);

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

	context->DSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->DSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->DSSetShaderResources(0, 1, m_floorDisplacementTexture.GetAddressOf());
	context->DSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	context->DSSetShader(
		m_domainShader.Get(),
		nullptr,
		0
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_floorPixelShader.Get(),
		nullptr,
		0
	);

	context->PSSetShaderResources(0, 1, m_floorTexture.GetAddressOf());
	context->PSSetShaderResources(1, 1, m_floorNormalTexture.GetAddressOf());
	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	//context->RSSetState(m_cullFrontState.Get());

	//Draw the quad
	context->Draw(4, 0);

	context->RSSetState(nullptr);

	context->HSSetShader(nullptr,
		nullptr, 0);

	context->DSSetShader(nullptr,
		nullptr,
		0);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

//Third, draw the pillars on top of the floor using ray marching
//----------------------------------------------------------------------------------------------------------------------------------------------

	// Each vertex is one instance of the VertexPositionColor struct.
	stride = sizeof(VertexPositionColor);
	offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_cubeVertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_canvasVertexShader.Get(),
		nullptr,
		0
	);

	context->PSSetShaderResources(0, 1, m_shaderResourceView.GetAddressOf());
	context->PSSetShaderResources(1, 1, m_wallTexture.GetAddressOf());
	context->PSSetShaderResources(2, 1, m_wallHeightTexture.GetAddressOf());
	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	// Attach our pixel shader.
	context->PSSetShader(
		m_pillarPixelShader.Get(),
		nullptr,
		0
	);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);

//Draw explicit models from vertex buffers
//----------------------------------------------------------------------------------------------------------------------------------------------

	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	stride = sizeof(VertexPositionTextureNTB);
	offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_snakeVertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_modelInputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_modelVertexShader.Get(),
		nullptr,
		0
	);

	model = XMMatrixScaling(3, 3, 3) * XMMatrixRotationX(-90)* XMMatrixTranslation(1.5f, -2.5f, 0.0f);

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

	context->PSSetShaderResources(0, 1, m_scalesTexture.GetAddressOf());

	// Attach our pixel shader.
	context->PSSetShader(
		m_modelPixelShader.Get(),
		nullptr,
		0
	);

	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	// Draw the objects.
	context->Draw(m_vertexCount, 0);

	model = XMMatrixScaling(3, 3, 3) * XMMatrixRotationX(-90)* XMMatrixTranslation(-1.5f, -2.5f, 0.0f);

	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(model));

	context->UpdateSubresource1(m_constantBuffer.Get(),	0, NULL, &m_constantBufferData,	0, 0, 0	);

	// Draw the objects.
	context->Draw(m_vertexCount, 0);

	//Draw the particles using geometry shader
	//----------------------------------------------------------------------------------------------------------------------------------------------

	// Each vertex is one instance of the VertexPositionColor struct.
	stride = sizeof(Particle);
	offset = 0;
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

	model = XMMatrixIdentity() * XMMatrixScaling(3, 3, 3);

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
	);
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	auto d3dDevice = m_deviceResources->GetD3DDevice();
	
	//Canvas vertex shader & input layout
	{
		auto fileData = DX::ReadData(L"SampleVertexShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreateVertexShader(fileData.data(), fileData.size(), nullptr, &m_canvasVertexShader));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			d3dDevice->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				fileData.data(),
				fileData.size(),
				&m_inputLayout
			)
		);

		D3D11_SAMPLER_DESC samplerDesc;
		//Create texture sampler state description
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		//Create texture sampler state
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_samplerState)
		);
	}

	//Room pixel shader and constant buffers
	{
		auto fileData = DX::ReadData(L"RoomPixelShader.cso");
		DX::ThrowIfFailed(
			d3dDevice->CreatePixelShader(
				fileData.data(),
				fileData.size(),
				nullptr,
				&m_roomPixelShader
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);

		CD3D11_BUFFER_DESC changesOnResizeConstantBufferDesc(sizeof(ChangesOnResizeConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&changesOnResizeConstantBufferDesc,
				nullptr,
				&m_changesOnResizeConstantBuffer
			)
		);

		CD3D11_BUFFER_DESC psConstantBufferDesc(sizeof(PixelShaderConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&psConstantBufferDesc,
				nullptr,
				&m_psConstantBuffer
			)
		);

		CD3D11_BUFFER_DESC timeBufferDesc(sizeof(ConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&timeBufferDesc,
				nullptr,
				&m_timeBuffer
			)
		);
	}

	//Pillar pixel shader
	{
		auto fileData = DX::ReadData(L"PillarPixelShader.cso");
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pillarPixelShader
			)
		);
	}
	
	m_psConstantBufferData.eye = XMFLOAT4(0.0f, 3.5f, 5.0f, 1.0f);
	m_psConstantBufferData.nearPlane = 1.0f;
	m_psConstantBufferData.farPlane = 100.0f;
	m_psConstantBufferData.lightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_psConstantBufferData.lightPos[0] = XMFLOAT4(-10.0f, 10.0f, -50.0f, 1.0f);
	m_psConstantBufferData.lightPos[1] = XMFLOAT4(10.0f, 10.0f, 50.0f, 1.0f);
	m_psConstantBufferData.lightPos[2] = XMFLOAT4(0.0f, 60.0f, 5.0f, 1.0f);
	m_psConstantBufferData.backgroundColor = XMFLOAT4(0.1f, 0.2f, 0.3f, 1.0f);
	m_psConstantBufferData.padding = XMFLOAT2();
	
	//Model Shaders
	{
		auto vsData = DX::ReadData(L"ModelVertexShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &m_modelVertexShader));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			d3dDevice->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				vsData.data(),
				vsData.size(),
				&m_modelInputLayout
			)
		);

		auto psData = DX::ReadData(L"ModelPixelShader.cso");
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				psData.data(),
				psData.size(),
				nullptr,
				&m_modelPixelShader
			)
		);
	}

	//Particle Shaders
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
	}

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

		D3D11_DEPTH_STENCIL_DESC depthDesc;
		ZeroMemory(&depthDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		DX::ThrowIfFailed(d3dDevice->CreateDepthStencilState(&depthDesc, &m_noWriteDepthState));

		D3D11_RASTERIZER_DESC cullDesc;
		cullDesc.FillMode = D3D11_FILL_SOLID;
		cullDesc.CullMode = D3D11_CULL_BACK;
		DX::ThrowIfFailed(d3dDevice->CreateRasterizerState(&cullDesc, &m_DisableCullState));
	}

	//Floor shaders
	{
		auto vsData = DX::ReadData(L"VertexShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &m_groundVertexShader));

		auto hsData = DX::ReadData(L"HullShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreateHullShader(hsData.data(), hsData.size(), nullptr, &m_hullShader));

		auto dsData = DX::ReadData(L"DomainShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreateDomainShader(dsData.data(), dsData.size(), nullptr, &m_domainShader));

		auto psData = DX::ReadData(L"FloorPixelShader.cso");
		DX::ThrowIfFailed(d3dDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &m_floorPixelShader));
	}

	//Textures
	{
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\Scales2.DDS", nullptr, &m_scalesTexture));
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\Stone_Wall_002_COLOR.DDS", nullptr, &m_floorTexture));
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\Stone_Wall_002_DISP.DDS", nullptr, &m_floorDisplacementTexture));
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\Stone_Wall_002_NRM.DDS", nullptr, &m_floorNormalTexture));
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\StoneWall_1024_albedo.DDS", nullptr, &m_wallTexture));
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\StoneWall_1024_normal.DDS", nullptr, &m_wallHeightTexture));
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\fire.DDS", nullptr, &m_fireTexture));
		DX::ThrowIfFailed(CreateDDSTextureFromFile(d3dDevice, L"Assets\\Textures\\noise.DDS", nullptr, &m_noiseTexture));
	}

	//Cube Geometry
	{
		static const VertexPositionColor cubeVertices[] = {
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = cubeVertices;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_cubeVertexBuffer));

		static const unsigned short cubeIndices[] = {
			0,2,1, 1,2,3,
			4,5,6, 5,7,6,
			0,1,5, 0,5,4,
			2,6,7, 2,7,3,
			0,4,6, 0,6,2,
			1,3,7, 1,7,5,
		};
		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = cubeIndices;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(d3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_indexBuffer));
	}

	//Quad Geometry
	{
		static const VertexPositionTextureNTB quadVertices[] = {
			{ XMFLOAT3(-1.0f, 0.0f,  1.0f), XMFLOAT2(0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT2(0, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 0.0f,  1.0f), XMFLOAT2(1, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 0.0f, -1.0f), XMFLOAT2(1, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = quadVertices;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_quadVertexBuffer));

		D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		d3dDevice->CreateRasterizerState(&rasterizerDesc, m_wireframeState.GetAddressOf());

		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_FRONT;
		d3dDevice->CreateRasterizerState(&rasterizerDesc, m_cullFrontState.GetAddressOf());
	}

	//Snake geometry
	{
		std::ifstream fin("Assets/Models/Snake.txt");
		if (fin.is_open())
		{
			char input = ' ';
			fin.get(input);
			while (input != ':') { fin.get(input); }

			int count;
			fin >> count;
			fin.get(input);

			while (input != ':') { fin.get(input); }
			fin.get(input); fin.get(input);

			std::vector<VertexPositionTextureNTB> vertices;
			float x, y, z;
			VertexPositionTextureNTB vertex;

			for (int i = 0; i < count; i++)
			{
				fin >> x >> y >> z; vertex.position = DirectX::XMFLOAT3(x, y, z);
				fin >> x >> y;      vertex.texture = DirectX::XMFLOAT2(x, y);
				fin >> x >> y >> z; vertex.normal = DirectX::XMFLOAT3(x, y, z);
				fin >> x >> y >> z; vertex.tangent = DirectX::XMFLOAT3(x, y, z);
				fin >> x >> y >> z; vertex.binormal = DirectX::XMFLOAT3(x, y, z);
				vertices.push_back(vertex);
			}
			fin.close();

			m_vertexCount = count;

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = static_cast<UINT>(sizeof(VertexPositionTextureNTB) * vertices.size());
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			d3dDevice->CreateBuffer(&bd, NULL, &m_snakeVertexBuffer);

			D3D11_MAPPED_SUBRESOURCE ms;
			m_deviceResources->GetD3DDeviceContext()->Map(m_snakeVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			memcpy(ms.pData, vertices.data(), sizeof(VertexPositionTextureNTB) * vertices.size());
			m_deviceResources->GetD3DDeviceContext()->Unmap(m_snakeVertexBuffer.Get(), 0);
		}
	}

	//Particles
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

	m_loadingComplete = true;
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_canvasVertexShader.Reset();
	m_particleVertexShader.Reset();
	m_particleVertexShaderSO.Reset();
	m_modelVertexShader.Reset();
	m_inputLayout.Reset();
	m_modelInputLayout.Reset();
	m_particleInputLayout.Reset();
	m_roomPixelShader.Reset();
	m_pillarPixelShader.Reset();
	m_modelPixelShader.Reset();
	m_particlePixelShader.Reset();
	m_particleGeometryShader.Reset();
	m_constantBuffer.Reset();
	m_psConstantBuffer.Reset();
	m_changesOnResizeConstantBuffer.Reset();
	m_cubeVertexBuffer.Reset();
	m_quadVertexBuffer.Reset();
	m_snakeVertexBuffer.Reset();
	m_particleVertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_additiveBlend.Reset();
	m_noWriteDepthState.Reset();
	m_groundVertexShader.Reset();
	m_hullShader.Reset();
	m_domainShader.Reset();
	m_wireframeState.Reset();
	m_renderTargetTexture.Reset();
	m_renderTargetView.Reset();
	m_shaderResourceView.Reset();
	m_samplerState.Reset();
	m_scalesTexture.Reset();
	m_floorTexture.Reset();
	m_cullFrontState.Reset();
	m_floorDisplacementTexture.Reset();
	m_floorNormalTexture.Reset();
	m_floorPixelShader.Reset();
	m_wallHeightTexture.Reset();
	m_wallTexture.Reset();
	m_fireTexture.Reset();
	m_noiseTexture.Reset();
	m_particleVertexBufferSO.Reset();
}