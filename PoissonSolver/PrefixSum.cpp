//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "PrefixSum.h"

#ifndef V_RETURN
#define V_RETURN(x)	{ hr = x; if (FAILED(hr)) return hr; }
#endif

PrefixSum::PrefixSum(ID3D11DeviceContext *pDeviceContext)
	: m_pd3dContext(pDeviceContext), m_uRefCount(1)
{
	m_pd3dContext->AddRef();
	m_pd3dContext->GetDevice(&m_pd3dDevice);
}

PrefixSum::~PrefixSum()
{
	if (m_pd3dContext) m_pd3dContext->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
}

HRESULT PrefixSum::Init(const uint32_t uSize)
{
	HRESULT hr;
	ID3DBlob *shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1.cso", &shaderBuffer));

	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pShaders[CS_PREFIXSUM1_FLOAT]
	);

	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum2.cso", &shaderBuffer));

	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pShaders[CS_PREFIXSUM2_FLOAT]
	);

	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1i.cso", &shaderBuffer));

	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pShaders[CS_PREFIXSUM1_INT]
	);

	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum2i.cso", &shaderBuffer));

	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pShaders[CS_PREFIXSUM2_INT]
	);

	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1u.cso", &shaderBuffer));

	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pShaders[CS_PREFIXSUM1_UINT]
	);

	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum2u.cso", &shaderBuffer));

	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pShaders[CS_PREFIXSUM2_UINT]
	);

	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	V_RETURN(CreateStructuredBuffer(m_pd3dDevice, sizeof(uint32_t), uSize, nullptr, &m_pPreSumInc));
	V_RETURN(CreateBufferUAV(m_pd3dDevice, m_pPreSumInc, &m_pUAVPreSumInc));

	return hr;
}

void PrefixSum::Scan(const SCAN_DATA_TYPE dataType, const uint32_t uSize, ID3D11UnorderedAccessView *const pSrc, ID3D11UnorderedAccessView *const pDst)
{
	const auto UAVInitialCounts = 0u;
	uint32_t CS_PREFIXSUM1, CS_PREFIXSUM2;

	switch (dataType)
	{
	case SCAN_DATA_TYPE_FLOAT:
		CS_PREFIXSUM1 = CS_PREFIXSUM1_FLOAT;
		CS_PREFIXSUM2 = CS_PREFIXSUM2_FLOAT;
		break;
	case SCAN_DATA_TYPE_INT:
		CS_PREFIXSUM1 = CS_PREFIXSUM1_INT;
		CS_PREFIXSUM2 = CS_PREFIXSUM2_INT;
		break;
	default:
		CS_PREFIXSUM1 = CS_PREFIXSUM1_UINT;
		CS_PREFIXSUM2 = CS_PREFIXSUM2_UINT;
	}

	// 1 Group for 1024 consecutive counters.
	const auto uNumGroups = static_cast<uint32_t>(ceil(uSize / 1024.0f));

	// Prefix sum
	m_pd3dContext->CSSetUnorderedAccessViews(0, 1, &pSrc, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(1, 1, &m_pUAVPreSumInc, &UAVInitialCounts);
	m_pd3dContext->CSSetShader(m_pShaders[CS_PREFIXSUM1], nullptr, 0);
	m_pd3dContext->Dispatch(uNumGroups, 1, 1);

	m_pd3dContext->CSSetUnorderedAccessViews(0, 1, &pDst, &UAVInitialCounts);
	m_pd3dContext->CSSetShader(m_pShaders[CS_PREFIXSUM2], nullptr, 0);
	m_pd3dContext->Dispatch(uNumGroups, 1, 1);
}

void PrefixSum::AddRef()
{
	++m_uRefCount;
}

void PrefixSum::Release()
{
	if (--m_uRefCount < 1) delete this;
}

HRESULT PrefixSum::CreateScan(ID3D11DeviceContext *const pDeviceContext, const uint32_t uSize, PrefixSum **ppScan)
{
	auto &pScan = *ppScan;
	pScan = new PrefixSum(pDeviceContext);

	return pScan->Init(uSize);
}
