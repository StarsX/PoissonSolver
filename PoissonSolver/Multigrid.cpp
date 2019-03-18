//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "Multigrid.h"
#include "SharedConst.h"

#ifndef V_RETURN
#define V_RETURN(x)	{ hr = x; if (FAILED(hr)) return hr; }
#endif

using namespace DirectX;
//using namespace std;

Multigrid::Multigrid(ID3D11DeviceContext *const pDeviceContext) :
	Jacobi(pDeviceContext)
{
}

Multigrid::~Multigrid()
{
}

HRESULT Multigrid::CreateSolver(ID3D11DeviceContext *const pDeviceContext, Multigrid **ppSolver)
{
	auto &pSolver = *ppSolver;
	pSolver = new Multigrid(pDeviceContext);
	return pSolver->Init();
}

HRESULT Multigrid::Init()
{
	HRESULT h, hr = Jacobi::Init();
	V_RETURN(hr);

	ID3DBlob *shaderBuffer = nullptr;
	D3D11_SHADER_INPUT_BIND_DESC desc;

	V_RETURN(D3DReadFileToBlob(L"CSDownSample.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pDownSmpShader);
	if (SUCCEEDED(hr))
	{
		ID3D11ShaderReflection *pReflector = nullptr;
		hr = D3DReflect(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
			IID_ID3D11ShaderReflection, (void**)&pReflector);
		if (SUCCEEDED(hr))
		{
			h = pReflector->GetResourceBindingDescByName("x", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_xdown = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("y", &desc);
			if (SUCCEEDED(h))m_uUAVSlot_ydown = desc.BindPoint;
			else hr = h;
		}
		if (pReflector) pReflector->Release();
	}
	if (shaderBuffer) shaderBuffer->Release();
	V_RETURN(hr);

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSUpSample.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pUpSmpShader);
	if (SUCCEEDED(hr))
	{
		ID3D11ShaderReflection *pReflector = nullptr;
		hr = D3DReflect(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
			IID_ID3D11ShaderReflection, (void**)&pReflector);
		if (SUCCEEDED(hr))
		{
			h = pReflector->GetResourceBindingDescByName("x", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_xup = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("y", &desc);
			if (SUCCEEDED(h))m_uUAVSlot_yup = desc.BindPoint;
			else hr = h;
		}
		if (pReflector) pReflector->Release();
	}
	if (shaderBuffer) shaderBuffer->Release();

	return hr;
}

void Multigrid::Solve(const XMUINT3 & vSize, ID3D11ShaderResourceView* const* const ppSrcSRVs,
	ID3D11ShaderResourceView* const* const ppDstSRVs, ID3D11UnorderedAccessView* const* const ppSrcUAVs,
	ID3D11UnorderedAccessView* const* const ppDstUAVs, uint32_t uNumIt, uint32_t uMips)
{
	const auto UAVInitialCounts = 0u;

	// Down sampling
	for (auto i = 0u; i < uMips - 1; ++i)
	{
		// Setup
		m_pd3dContext->CSSetShaderResources(m_uUAVSlot_xdown, 1, &ppSrcSRVs[i]);
		m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_ydown, 1, &ppSrcUAVs[i + 1], &UAVInitialCounts);

		m_pd3dContext->CSSetShader(m_pDownSmpShader, nullptr, 0);
		m_pd3dContext->Dispatch(vSize.x / 2, vSize.y / 2, vSize.z / 2);
	}

	for (int i = uMips - 1; i > 0; --i)
	{
		// Setup b
		m_pd3dContext->CSSetShaderResources(m_uSRVSlot, 1, &ppSrcSRVs[i]);

		// Jacobi iterations
		for (auto j = 0u; j < uNumIt >> (i + 4); ++j) jacobi(vSize, ppDstUAVs[i]);

		// Up sampling
		m_pd3dContext->CSSetShaderResources(m_uUAVSlot_xup, 1, &ppDstSRVs[i]);
		m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_yup, 1, &ppDstUAVs[i - 1], &UAVInitialCounts);
		m_pd3dContext->CSSetShader(m_pUpSmpShader, nullptr, 0);
		m_pd3dContext->Dispatch(vSize.x / THREAD_GROUP_SIZE, vSize.y / THREAD_GROUP_SIZE, vSize.z / THREAD_GROUP_SIZE);
	}

	Jacobi::Solve(vSize, *ppSrcSRVs, *ppDstUAVs, uNumIt);
}
