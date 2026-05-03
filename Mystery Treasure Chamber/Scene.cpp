#include "pch.h"
#include "Material.h"
#include "MeshObject.h"
#include "Scene.h"

#include <fstream>

using namespace Mystery_Treasure_Chamber;
using namespace DirectX;

bool Scene::Initialize(ID3D11Device* device)
{
	m_d3dDevice = device;

    D3D11_BUFFER_DESC vsDesc = {};
    vsDesc.Usage = D3D11_USAGE_DEFAULT;
    vsDesc.ByteWidth = sizeof(Mystery_Treasure_Chamber::ModelViewProjectionConstantBuffer);
    vsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&vsDesc, nullptr, m_mvpConstantBuffer.GetAddressOf()))) return false;

    D3D11_BUFFER_DESC psDesc = {};
    psDesc.Usage = D3D11_USAGE_DEFAULT;
    psDesc.ByteWidth = sizeof(Mystery_Treasure_Chamber::PixelShaderConstantBuffer);
    psDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&psDesc, nullptr, m_psConstantBuffer.GetAddressOf()))) return false;

    D3D11_BUFFER_DESC sizeBufferDesc = {};
    sizeBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    sizeBufferDesc.ByteWidth = sizeof(Mystery_Treasure_Chamber::ChangesOnResizeConstantBuffer);
    sizeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&sizeBufferDesc, nullptr, m_sizeConstantBuffer.GetAddressOf()))) return false;

    D3D11_BUFFER_DESC timeBufferDesc = {};
    timeBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    timeBufferDesc.ByteWidth = sizeof(Mystery_Treasure_Chamber::ConstantBuffer);
    timeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&timeBufferDesc, nullptr, m_timeConstantBuffer.GetAddressOf()))) return false;

	SetupSceneObjects();

	return true;
}

bool Scene::CreateRoomRenderTarget(ID3D11Device* device, UINT in_width, UINT in_height)
{
	HRESULT hr;
	float width = static_cast<float>(in_width);
	float height = static_cast<float>(in_height);
	float aspectRatio = width / height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	ScreenSizeData.height = height;
	ScreenSizeData.width = width;

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
		&MatrixData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 3.5f, 5.0f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&MatrixData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));

	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// Create the render target texture.
	hr = device->CreateTexture2D(&textureDesc, NULL, &m_roomTexture);
	if (FAILED(hr))
	{
		return false;
	}

	// Setup the description of the render target view.
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	hr = device->CreateRenderTargetView(m_roomTexture.Get(), &renderTargetViewDesc, m_roomRTV.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	// Setup the description of the shader resource view.
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view.
	hr = device->CreateShaderResourceView(m_roomTexture.Get(), &shaderResourceViewDesc, m_roomSRV.GetAddressOf());
}

void Scene::SetupSceneObjects()
{
	//Set up PS constant buffer
	LightingData.eye = XMFLOAT4(0.0f, 3.5f, 5.0f, 1.0f);
	LightingData.nearPlane = 1.0f;
	LightingData.farPlane = 100.0f;
	LightingData.lightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	LightingData.lightPos[0] = XMFLOAT4(-10.0f, 10.0f, -50.0f, 1.0f);
	LightingData.lightPos[1] = XMFLOAT4(10.0f, 10.0f, 50.0f, 1.0f);
	LightingData.lightPos[2] = XMFLOAT4(0.0f, 60.0f, 5.0f, 1.0f);
	LightingData.backgroundColor = XMFLOAT4(0.1f, 0.2f, 0.3f, 1.0f);
	LightingData.padding = XMFLOAT2();

	//Create the room
	auto roomMat = std::make_shared<Material>();
	roomMat->Initialize(
		m_d3dDevice.Get(),
		L"SampleVertexShader.cso",
		L"RoomPixelShader.cso",
		L"Assets\\Textures\\StoneWall_1024_albedo.DDS",
		VertexFormat::PositionOnly);

	const std::vector<VertexPositionColor> cubeVertices = {
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	};

	const std::vector<unsigned short> cubeIndices = {
			0,2,1, 1,2,3,
			4,5,6, 5,7,6,
			0,1,5, 0,5,4,
			2,6,7, 2,7,3,
			0,4,6, 0,6,2,
			1,3,7, 1,7,5,
	};

	auto room = std::make_shared<MeshObject>(
		m_d3dDevice.Get(),
		cubeVertices.data(), sizeof(VertexPositionColor), cubeVertices.size(), cubeIndices, roomMat);
	AddBackgroundObject(room);

	//Add the floor
	auto floorMat = std::make_shared<Material>();
	floorMat->Initialize(m_d3dDevice.Get(), L"VertexShader.cso", L"FloorPixelShader.cso",
		L"Assets\\Textures\\Stone_Wall_002_COLOR.DDS",
		VertexFormat::StandardMesh,
		L"Assets\\Textures\\Stone_Wall_002_NRM.DDS",
		L"HullShader.cso", L"DomainShader.cso",
		L"Assets\\Textures\\Stone_Wall_002_DISP.DDS");

	const std::vector<VertexPositionTextureNTB> floorVerts = {
			{ XMFLOAT3(-1.0f, 0.0f,  1.0f), XMFLOAT2(0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT2(0, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 0.0f,  1.0f), XMFLOAT2(1, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 0.0f, -1.0f), XMFLOAT2(1, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
	};

	const std::vector<unsigned short> floorIndices = { 0, 1, 2, 3 };
	auto floor = std::make_shared<MeshObject>(
		m_d3dDevice.Get(), floorVerts.data(), sizeof(VertexPositionTextureNTB), floorVerts.size(), floorIndices,
		floorMat, D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	floor->Position = { 0.0f, -2.5f, 0.0f };
	floor->Scale = { 5.5f, 5.5f, 5.0f };
	AddBackgroundObject(floor);

	//Add raymarched pillars as mesh objects
	auto pillarMat = std::make_shared < Material>();
	pillarMat->Initialize(m_d3dDevice.Get(),
		L"SampleVertexShader.cso",
		L"PillarPixelShader.cso",
		L"Assets\\Textures\\StoneWall_1024_albedo.DDS",
		VertexFormat::PositionOnly,
		L"Assets\\Textures\\StoneWall_1024_normal.DDS");

	auto pillars = std::make_shared<MeshObject>(
		m_d3dDevice.Get(),
		cubeVertices.data(), sizeof(VertexPositionColor), cubeVertices.size(), cubeIndices, pillarMat);
	AddMeshObject(pillars);

	//Add snakes to the scene
	auto snakeMaterial = std::make_shared<Material>();
	snakeMaterial->Initialize(
		m_d3dDevice.Get(),
		L"ModelVertexShader.cso",
		L"ModelPixelShader.cso",
		L"Assets\\Textures\\Scales2.DDS"
	);


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
		std::vector<unsigned short> snakeIndices;
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
			snakeIndices.push_back(i);
		}
		fin.close();

		auto leftSnake = std::make_shared<MeshObject>(
			m_d3dDevice.Get(),
			vertices.data(),
			sizeof(VertexPositionTextureNTB),
			(UINT)vertices.size(),
			snakeIndices,
			snakeMaterial
		);
		leftSnake->Position = { -1.5f, -2.5f, 0.0f };
		leftSnake->Rotation = { -90.f, 0.f, 0.f };
		leftSnake->Scale = { 3.0f, 3.0f, 3.0f };
		AddMeshObject(leftSnake);

		auto rightSnake = std::make_shared<MeshObject>(
			m_d3dDevice.Get(),
			vertices.data(),
			sizeof(VertexPositionTextureNTB),
			(UINT)vertices.size(),
			snakeIndices,
			snakeMaterial
		);
		rightSnake->Position = { 1.5f, -2.5f, 0.0f };
		rightSnake->Rotation = { -90.f, 0.f, 0.f };
		rightSnake->Scale = { 3.0f, 3.0f, 3.0f };
		AddMeshObject(rightSnake);
	}
}

void::Scene::AddMeshObject(std::shared_ptr<SceneObject> obj)
{
    m_meshObjects.push_back(obj);
}

void Scene::AddBackgroundObject(std::shared_ptr<SceneObject> obj)
{
    m_backgroundObjects.push_back(obj);
}

void Scene::Update(const DX::StepTimer& timer)
{
	TimeData.time = static_cast<float>(timer.GetTotalSeconds());
	TimeData.deltaTime = static_cast<float>(timer.GetElapsedSeconds());
}

void Scene::Render(ID3D11DeviceContext* context)
{
	context->UpdateSubresource(m_psConstantBuffer.Get(), 0, nullptr, &LightingData, 0, 0);
	context->PSSetConstantBuffers(0, 1, m_psConstantBuffer.GetAddressOf());

	context->UpdateSubresource(m_sizeConstantBuffer.Get(), 0, nullptr, &ScreenSizeData, 0, 0);
	context->VSSetConstantBuffers(1, 1, m_sizeConstantBuffer.GetAddressOf());

	context->UpdateSubresource(m_timeConstantBuffer.Get(), 0, nullptr, &TimeData, 0, 0);
	context->VSSetConstantBuffers(2, 1, m_timeConstantBuffer.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mainRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mainDSV;
	context->OMGetRenderTargets(1, mainRTV.GetAddressOf(), mainDSV.GetAddressOf());

	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	context->PSSetShaderResources(2, 1, nullSRV);

	context->OMSetRenderTargets(1, m_roomRTV.GetAddressOf(), mainDSV.Get());

	for (const auto& bg : m_backgroundObjects)
	{
		// Update ONLY the model matrix inside your existing MatrixData struct
		DirectX::XMStoreFloat4x4(&MatrixData.model, DirectX::XMMatrixTranspose(bg->GetModelMatrix()));

		// Upload the entire MVP struct to the GPU
		context->UpdateSubresource(m_mvpConstantBuffer.Get(), 0, nullptr, &MatrixData, 0, 0);
		context->VSSetConstantBuffers(0, 1, m_mvpConstantBuffer.GetAddressOf());

		bg->Draw(context);

		context->ClearDepthStencilView(mainDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	context->OMSetRenderTargets(1, mainRTV.GetAddressOf(), mainDSV.Get());
	context->ClearDepthStencilView(mainDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->PSSetShaderResources(2, 1, m_roomSRV.GetAddressOf());

	//Pillars
	//{
	//	DirectX::XMStoreFloat4x4(&MatrixData.model, DirectX::XMMatrixTranspose(m_pillars->GetModelMatrix()));

	//	// Upload the entire MVP struct to the GPU
	//	context->UpdateSubresource(m_mvpConstantBuffer.Get(), 0, nullptr, &MatrixData, 0, 0);
	//	context->VSSetConstantBuffers(0, 1, m_mvpConstantBuffer.GetAddressOf());

	//	m_pillars->Draw(context);
	//}

    for (const auto& obj : m_meshObjects)
    {
        // Update ONLY the model matrix inside your existing MatrixData struct
        DirectX::XMStoreFloat4x4(&MatrixData.model, DirectX::XMMatrixTranspose(obj->GetModelMatrix()));

        // Upload the entire MVP struct to the GPU
        context->UpdateSubresource(m_mvpConstantBuffer.Get(), 0, nullptr, &MatrixData, 0, 0);
        context->VSSetConstantBuffers(0, 1, m_mvpConstantBuffer.GetAddressOf());
        context->DSSetConstantBuffers(0, 1, m_mvpConstantBuffer.GetAddressOf());

        obj->Draw(context);

		context->ClearDepthStencilView(mainDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
}