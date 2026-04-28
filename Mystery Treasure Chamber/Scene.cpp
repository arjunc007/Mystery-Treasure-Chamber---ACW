#include "pch.h"
#include "Scene.h"

bool Scene::Initialize(ID3D11Device* device)
{
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

    D3D11_BUFFER_DESC timeBufferDesc = {};
    timeBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    timeBufferDesc.ByteWidth = sizeof(Mystery_Treasure_Chamber::ConstantBuffer);
    timeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&timeBufferDesc, nullptr, m_timeConstantBuffer.GetAddressOf()))) return false;

	return true;
}

void::Scene::AddMeshObject(std::shared_ptr<SceneObject> obj)
{
    m_meshObjects.push_back(obj);
}

void Scene::AddBackgroundObject(std::shared_ptr<SceneObject> obj)
{
    m_backgroundObjects.push_back(obj);
}

void Scene::Update(float deltaTime)
{

}

void Scene::Render(ID3D11DeviceContext* context)
{
    context->UpdateSubresource(m_psConstantBuffer.Get(), 0, nullptr, &LightingData, 0, 0);
    context->PSSetConstantBuffers(0, 1, m_psConstantBuffer.GetAddressOf());

    for (const auto& bg : m_backgroundObjects)
    {
        // Update ONLY the model matrix inside your existing MatrixData struct
        DirectX::XMStoreFloat4x4(&MatrixData.model, DirectX::XMMatrixTranspose(bg->GetModelMatrix()));

        // Upload the entire MVP struct to the GPU
        context->UpdateSubresource(m_mvpConstantBuffer.Get(), 0, nullptr, &MatrixData, 0, 0);
        context->VSSetConstantBuffers(0, 1, m_mvpConstantBuffer.GetAddressOf());

        bg->Draw(context);
    }

    for (const auto& obj : m_meshObjects)
    {
        // Update ONLY the model matrix inside your existing MatrixData struct
        DirectX::XMStoreFloat4x4(&MatrixData.model, DirectX::XMMatrixTranspose(obj->GetModelMatrix()));

        // Upload the entire MVP struct to the GPU
        context->UpdateSubresource(m_mvpConstantBuffer.Get(), 0, nullptr, &MatrixData, 0, 0);
        context->VSSetConstantBuffers(0, 1, m_mvpConstantBuffer.GetAddressOf());

        context->UpdateSubresource(m_timeConstantBuffer.Get(), 0, nullptr, &TimeData, 0, 0);
        context->VSSetConstantBuffers(2, 1, m_timeConstantBuffer.GetAddressOf());

        obj->Draw(context);
    }
}