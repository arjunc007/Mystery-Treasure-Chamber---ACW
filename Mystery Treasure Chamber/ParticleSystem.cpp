#include "pch.h"
#include "ParticleSystem.h"

using namespace Mystery_Treasure_Chamber;
using namespace DirectX;

ParticleSystem::ParticleSystem() : m_firstRun(true)
								, m_initialEmittersCount(4)
								, m_pingPongToggle(0), m_maxParticles(1000) {}

bool ParticleSystem::Initialize(ID3D11Device* device, const std::vector<XMFLOAT3>& emitterPositions)
{
	std::vector<Particle> seeds;

	for (const auto& pos : emitterPositions)
	{
		Particle p;
		p.position = pos;
		p.speed = XMFLOAT3(0, 0, 0);
		p.size = XMFLOAT2(1.0f, 1.0f);
		p.age = 0.0f;
		p.type = 0;
		seeds.push_back(p);
	}

	if (!CreateParticleBuffer(device, m_particleBuffers[0].GetAddressOf(), &seeds)) return false;

	if (!CreateParticleBuffer(device, m_particleBuffers[1].GetAddressOf())) return false;

	return true;
}

bool ParticleSystem::CreateParticleBuffer(ID3D11Device* device, ID3D11Buffer** buffer, const std::vector<Particle>* initialData)
{
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Particle) * m_maxParticles;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	if (initialData)
	{
		std::vector<Particle> paddedData(m_maxParticles);

		for (size_t i = 0; i < initialData->size(); ++i)
		{
			paddedData[i] = (*initialData)[i];
		}

		D3D11_SUBRESOURCE_DATA vinitData = {};
		vinitData.pSysMem = paddedData.data();
		return SUCCEEDED(device->CreateBuffer(&vbd, &vinitData, buffer));
	}
	else
	{
		return SUCCEEDED(device->CreateBuffer(&vbd, nullptr, buffer));
	}
}

void ParticleSystem::UpdateAndRender(ID3D11DeviceContext* context,
	ID3D11Buffer* timeBuffer,
	ID3D11Buffer* matrixBuffer,
	ID3D11Buffer* resizeBuffer,
	ID3D11Buffer* viewBuffer)
{
	if (!m_material) return;

	UINT stride = sizeof(Particle);
	UINT offset = 0;

	ID3D11Buffer* bufferRead = m_particleBuffers[m_pingPongToggle].Get();
	ID3D11Buffer* bufferWrite = m_particleBuffers[!m_pingPongToggle].Get();

	m_material->BindForUpdate(context, timeBuffer);

	context->IASetVertexBuffers(0, 1, &bufferRead, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->SOSetTargets(1, &bufferWrite, &offset);

	if (m_firstRun)
	{
		context->Draw(m_initialEmittersCount, 0);
		m_firstRun = false;
	}
	else
	{
		context->DrawAuto();
	}

	ID3D11Buffer* nullBuffer = nullptr;
	context->SOSetTargets(1, &nullBuffer, &offset);

	m_material->BindForRender(context, matrixBuffer, resizeBuffer, viewBuffer);

	context->IASetVertexBuffers(0, 1, &bufferWrite, &stride, &offset);
	context->DrawAuto();

	m_material->Unbind(context);
	m_pingPongToggle = !m_pingPongToggle;
}