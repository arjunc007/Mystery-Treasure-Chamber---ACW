#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"
#include "DDSTextureLoader.h"
//#include "..\Common\BasicShapes.h"

#include <fstream>

using namespace Mystery_Treasure_Chamber;

using namespace DirectX;
using namespace Windows::Foundation;

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
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	m_changesOnResizeConstantBufferData.height = outputSize.Height;
	m_changesOnResizeConstantBufferData.width = outputSize.Width;

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

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

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
	textureDesc.Width = outputSize.Width;
	textureDesc.Height = outputSize.Height;
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
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"RoomPixelShader.cso");
	auto loadPSTask2 = DX::ReadDataAsync(L"PillarPixelShader.cso");
	auto loadModelVS = DX::ReadDataAsync(L"ModelVertexShader.cso");
	auto loadModelPS = DX::ReadDataAsync(L"ModelPixelShader.cso");
	auto loadFloorPS = DX::ReadDataAsync(L"FloorPixelShader.cso");
	auto loadParticleVSSO = DX::ReadDataAsync(L"ParticleVertexShaderSO.cso");
	auto loadParticleVS = DX::ReadDataAsync(L"ParticleVertexShader.cso");
	auto loadParticlePS = DX::ReadDataAsync(L"ParticlePixelShader.cso");
	auto loadGSTask = DX::ReadDataAsync(L"GeometryShader.cso");
	auto loadGroundVS = DX::ReadDataAsync(L"VertexShader.cso");
	auto loadHSTask = DX::ReadDataAsync(L"HullShader.cso");
	auto loadDSTask = DX::ReadDataAsync(L"DomainShader.cso");
	auto loadGSSOTask = DX::ReadDataAsync(L"GeometryShaderSO.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_canvasVertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
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
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
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
	});

	auto createPSTask2 = loadPSTask2.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pillarPixelShader
			)
		);
	});

	m_psConstantBufferData.eye = XMFLOAT4(0.0f, 3.5f, 5.0f, 1.0f);
	m_psConstantBufferData.nearPlane = 1.0f;
	m_psConstantBufferData.farPlane = 100.0f;
	m_psConstantBufferData.lightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_psConstantBufferData.lightPos[0] = XMFLOAT4(-10.0f, 10.0f, -50.0f, 1.0f);
	m_psConstantBufferData.lightPos[1] = XMFLOAT4(10.0f, 10.0f, 50.0f, 1.0f);
	m_psConstantBufferData.lightPos[2] = XMFLOAT4(0.0f, 60.0f, 5.0f, 1.0f);
	m_psConstantBufferData.backgroundColor = XMFLOAT4(0.1f, 0.2f, 0.3f, 1.0f);
	m_psConstantBufferData.padding = XMFLOAT2();

	auto createModelVS = loadModelVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_modelVertexShader
			)
		);
		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_modelInputLayout
			)
		);
	});

	auto createModelPS = loadModelPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_modelPixelShader
			)
		);
	});

	auto createParticleVSTask = loadParticleVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_particleVertexShader
			)
		);
	});

	auto createParticleVSSOTask = loadParticleVSSO.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_particleVertexShaderSO
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 3, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_particleInputLayout
			)
		);
	});

	auto createFloorPSTask = loadFloorPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_floorPixelShader
			)
		);
	});

	auto createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_particleGeometryShader
			)
		);
	});

	auto createGSSOTask = (loadGSSOTask).then([this](const std::vector<byte>& fileData) {
		D3D11_SO_DECLARATION_ENTRY pDecl[] =
		{
			// semantic name, semantic index, start component, component count, output slot
			{ 0, "POSITION", 0, 0, 3, 0 },   // output all components of position
			{ 0, "TEXCOORD", 0, 0, 3, 0 },     // output the first 3 of the normal
			{ 0, "TEXCOORD", 1, 0, 2, 0 },		// output the first 2 texture coordinates
			{ 0, "TEXCOORD", 2, 0, 1, 0 },
			{ 0, "TEXCOORD", 3, 0, 1, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShaderWithStreamOutput(
				&fileData[0],
				fileData.size(),
				pDecl,
				ARRAYSIZE(pDecl),
				NULL,
				0,
				D3D11_SO_NO_RASTERIZED_STREAM,
				nullptr,
				&m_particleGeometryShaderSO
				)
		);
	});

	auto createParticlePS = loadParticlePS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_particlePixelShader
			)
		);

		//Create additive blend state
		D3D11_BLEND_DESC additiveBlendDesc;
		ZeroMemory(&additiveBlendDesc, sizeof(D3D11_BLEND_DESC));
		// Create an alpha enabled blend state description.
		additiveBlendDesc.AlphaToCoverageEnable = FALSE;
		additiveBlendDesc.RenderTarget[0].BlendEnable = TRUE;
		additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBlendState(
				&additiveBlendDesc,
				&m_additiveBlend
			)
		);

		//Create depth state for particles
		D3D11_DEPTH_STENCIL_DESC depthDesc;
		ZeroMemory(&depthDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateDepthStencilState(
				&depthDesc,
				&m_noWriteDepthState
			)
		);

		D3D11_RASTERIZER_DESC cullDesc;
		cullDesc.FillMode = D3D11_FILL_SOLID;
		cullDesc.CullMode = D3D11_CULL_BACK;

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateRasterizerState(
				&cullDesc,
				&m_DisableCullState
			)
		);
	});

	auto createGroundVSTask = loadGroundVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_groundVertexShader
			)
		);
	});

	auto createHSTask = loadHSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateHullShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_hullShader
			)
		);
	});

	auto createDSTask = loadDSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateDomainShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_domainShader
			)
		);
	});

	auto createTextureTask = (createModelVS && createModelPS).then([this]() {
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\Scales2.DDS", nullptr, &m_scalesTexture)
		);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\Stone_Wall_002_COLOR.DDS", nullptr, &m_floorTexture)
		);

		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\Stone_Wall_002_DISP.DDS", nullptr, &m_floorDisplacementTexture)
		);

		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\Stone_Wall_002_NRM.DDS", nullptr, &m_floorNormalTexture)
		);

		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\StoneWall_1024_albedo.DDS", nullptr, &m_wallTexture)
		);

		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\StoneWall_1024_normal.DDS", nullptr, &m_wallHeightTexture)
		);

		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\fire.DDS", nullptr, &m_fireTexture)
		);

		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets\\Textures\\noise.DDS", nullptr, &m_noiseTexture)
		);
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this]() {

		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionColor cubeVertices[] =
		{
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
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_cubeVertexBuffer
			)
		);

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		static const unsigned short cubeIndices[] =
		{
			0,2,1, // -x
			1,2,3,

			4,5,6, // +x
			5,7,6,

			0,1,5, // -y
			0,5,4,

			2,6,7, // +y
			2,7,3,

			0,4,6, // -z
			0,6,2,

			1,3,7, // +z
			1,7,5,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
			)
		);
	});

	auto createQuadTask = (createPSTask && createGroundVSTask).then([this]() {
		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionTextureNTB quadVertices[] =
		{
			{ XMFLOAT3(-1.0f, 0.0f, 1.0f), XMFLOAT2(0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 0.0f,  -1.0f), XMFLOAT2(0, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(1, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 0.0f,  -1.0f), XMFLOAT2(1, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = quadVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_quadVertexBuffer
			)
		);

		D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;

		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc,
			m_wireframeState.GetAddressOf());

		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_FRONT;

		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc,
			m_cullFrontState.GetAddressOf());
	});

	auto createSnakeTask = (createModelPS && createModelVS).then([this]() {
		std::ifstream fin("Assets/Models/Snake.txt");

		if (fin.fail())
			int c = 9;

		char input = ' ';

		fin.get(input);

		while (input != ':')
		{
			fin.get(input);
		}

		//Read number of vertices
		int count;

		fin >> count;

		fin.get(input);

		while (input != ':')
		{
			fin.get(input);
		}

		fin.get(input);
		fin.get(input);

		std::vector<VertexPositionTextureNTB> vertices;

		float x, y, z;
		VertexPositionTextureNTB vertex;

		for (int i = 0; i < count; i++)
		{
			//Position
			fin >> x >> y >> z;
			vertex.position = DirectX::XMFLOAT3(x, y, z);

			//Texture
			fin >> x >> y;
			vertex.texture = DirectX::XMFLOAT2(x, y);

			//Normal
			fin >> x >> y >> z;
			vertex.normal = DirectX::XMFLOAT3(x, y, z);

			//Tangent
			fin >> x >> y >> z;
			vertex.tangent = DirectX::XMFLOAT3(x, y, z);

			//Binormal
			fin >> x >> y >> z;
			vertex.binormal = DirectX::XMFLOAT3(x, y, z);

			vertices.push_back(vertex);
		}

		fin.close();

		m_vertexCount = count;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
		bd.ByteWidth = sizeof(VertexPositionTextureNTB) * vertices.size();   // size is the VERTEX struct * num vertices
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer

		m_deviceResources->GetD3DDevice()->CreateBuffer(&bd, NULL, &m_snakeVertexBuffer);        // create the buffer

																								 // copy the vertices into the buffer
		D3D11_MAPPED_SUBRESOURCE ms;
		m_deviceResources->GetD3DDeviceContext()->Map(m_snakeVertexBuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);		// map the buffer
		memcpy(ms.pData, vertices.data(), sizeof(VertexPositionTextureNTB) * vertices.size());			// copy the data
		m_deviceResources->GetD3DDeviceContext()->Unmap(m_snakeVertexBuffer.Get(), NULL);
	});

	auto createParticlesTask = (createParticlePS && createParticleVSTask && createGSTask).then([this]() {
		// Load mesh vertices. Each vertex has a position and a color.
		/*std::vector<Particle> particleVertices;

		particleVertices.push_back({ XMFLOAT3(0, 0, 0), XMFLOAT3(1.0f, 0.0f, 0.0f) });

		for (float y = 0.01f; y < .3f; y += 0.01f)
		{
			for (int theta = 0; theta < 360; theta += 5)
			{
				particleVertices.push_back({XMFLOAT3(y/2 * sin(XMConvertToRadians(theta)), y, y/2 * cos(XMConvertToRadians(theta))), XMFLOAT3(1.0f, 0.0f, 0.0f)});
			}
		}*/

		m_maxParticles = 1000;

		Particle particleVertices[] = {
			{XMFLOAT3(), XMFLOAT3(), XMFLOAT2(0.015f, 0.015f), 0.0f, 0.0f}
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = particleVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(particleVertices) * m_maxParticles, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_particleVertexBuffer
			)
		);

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				0,
				&m_particleVertexBufferSO
			)
		);
	});

	// Once everything is loaded, the object is ready to be rendered.
	(createCubeTask && createParticlesTask && createSnakeTask && createQuadTask && createTextureTask).then([this]() {
		m_loadingComplete = true;
	});
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