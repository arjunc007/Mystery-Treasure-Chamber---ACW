#include "pch.h"
#include "MeshObject.h"

MeshObject::MeshObject(ID3D11Device* device, const void* vertexData, UINT vertexSize, UINT vertexCount, const std::vector<unsigned short>& indices, std::shared_ptr<Material> material)
{
	m_material = material;
	m_vertexStride = vertexSize;
	m_indexCount = (UINT)indices.size();

	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = vertexSize * vertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vInitData = {};
	vInitData.pSysMem = vertexData;
	device->CreateBuffer(&vbd, &vInitData, m_vertexBuffer.GetAddressOf());

	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(unsigned short) * m_indexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA iInitData = {};
	iInitData.pSysMem = indices.data();
	device->CreateBuffer(&ibd, &iInitData, m_indexBuffer.GetAddressOf());
}

void MeshObject::Draw(ID3D11DeviceContext* context)
{
	m_material->Bind(context);

	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &m_vertexStride, &offset);
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->DrawIndexed(m_indexCount, 0, 0);
}