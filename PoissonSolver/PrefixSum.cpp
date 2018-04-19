//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "PrefixSum.h"

#ifndef V_RETURN
#define V_RETURN(x)	{ hr = x; if (FAILED(hr)) return hr; }
#endif

ID3D11UnorderedAccessView	*const	g_pNullUAV = nullptr;	// Helper to Clear UAVs

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
	HRESULT h, hr;
	ID3DBlob *shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM1_FLOAT]);
	if (SUCCEEDED(hr))
	{
		ID3D11ShaderReflection *pReflector = nullptr;
		hr = D3DReflect(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
			IID_ID3D11ShaderReflection, (void**)&pReflector);
		if (SUCCEEDED(hr))
		{
			D3D11_SHADER_INPUT_BIND_DESC desc;
			h = pReflector->GetResourceBindingDescByName("g_RWDst", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_Dst = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("g_RWInc", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_Inc = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("g_RWSrc", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_Src = desc.BindPoint;
			else hr = h;
		}
		if (pReflector) pReflector->Release();
	}
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1_rw.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM1_FLOAT_RW]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum2.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM2_FLOAT]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1i.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM1_INT]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1i_rw.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM1_INT_RW]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum2i.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM2_INT]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1u.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM1_UINT]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum1u_rw.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM1_UINT_RW]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSPrefixSum2u.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShaders[CS_PREFIXSUM2_UINT]);
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	V_RETURN(CreateStructuredBuffer(m_pd3dDevice, sizeof(uint32_t), uSize, nullptr, &m_pInc));
	V_RETURN(CreateBufferUAV(m_pd3dDevice, m_pInc, &m_pUAVInc));

	return hr;
}

void PrefixSum::Scan(const SCAN_DATA_TYPE dataType, const uint32_t uSize,
	ID3D11UnorderedAccessView *const pUAVSrc, ID3D11UnorderedAccessView *const pUAVDst)
{
	const auto UAVInitialCounts = 0u;
	uint32_t CS_PREFIXSUM1, CS_PREFIXSUM1_RW, CS_PREFIXSUM2;

	switch (dataType)
	{
	case SCAN_DATA_TYPE_FLOAT:
		CS_PREFIXSUM1 = CS_PREFIXSUM1_FLOAT;
		CS_PREFIXSUM1_RW = CS_PREFIXSUM1_FLOAT_RW;
		CS_PREFIXSUM2 = CS_PREFIXSUM2_FLOAT;
		break;
	case SCAN_DATA_TYPE_INT:
		CS_PREFIXSUM1 = CS_PREFIXSUM1_INT;
		CS_PREFIXSUM1_RW = CS_PREFIXSUM1_INT_RW;
		CS_PREFIXSUM2 = CS_PREFIXSUM2_INT;
		break;
	default:
		CS_PREFIXSUM1 = CS_PREFIXSUM1_UINT;
		CS_PREFIXSUM1_RW = CS_PREFIXSUM1_UINT_RW;
		CS_PREFIXSUM2 = CS_PREFIXSUM2_UINT;
	}

	// 1 Group for 1024 consecutive counters.
	const auto uNumGroups = static_cast<uint32_t>(ceil(uSize / 1024.0f));

	// Prefix sum
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Dst, 1, &pUAVDst, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Inc, 1, &m_pUAVInc, &UAVInitialCounts);
	if (pUAVSrc != pUAVDst)
		m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Src, 1, &pUAVSrc, &UAVInitialCounts);

	m_pd3dContext->CSSetShader(m_pShaders[pUAVSrc == pUAVDst ? CS_PREFIXSUM1_RW : CS_PREFIXSUM1], nullptr, 0);
	m_pd3dContext->Dispatch(uNumGroups, 1, 1);
	
	m_pd3dContext->CSSetShader(m_pShaders[CS_PREFIXSUM2], nullptr, 0);
	m_pd3dContext->Dispatch(uNumGroups, 1, 1);

	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Src, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Inc, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Dst, 1, &g_pNullUAV, &UAVInitialCounts);
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
