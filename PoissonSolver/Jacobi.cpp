//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "Jacobi.h"
#include "SharedConst.h"

#ifndef V_RETURN
#define V_RETURN(x)	{ hr = x; if (FAILED(hr)) return hr; }
#endif

using namespace DirectX;
using namespace std;

ID3D11ShaderResourceView	*const	g_pNullSRV = nullptr;	// Helper to Clear SRVs
ID3D11UnorderedAccessView	*const	g_pNullUAV = nullptr;	// Helper to Clear UAVs

Jacobi::Jacobi(ID3D11DeviceContext *const pDeviceContext)
	: m_pd3dContext(pDeviceContext), m_uRefCount(1)
{
	m_pd3dContext->AddRef();
	m_pd3dContext->GetDevice(&m_pd3dDevice);
}

Jacobi::~Jacobi()
{
	if (m_pShader) m_pShader->Release();
	if (m_pd3dContext) m_pd3dContext->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
}

HRESULT Jacobi::CreateSolver(ID3D11DeviceContext *const pDeviceContext, Jacobi **ppSolver)
{
	auto &pSolver = *ppSolver;
	pSolver = new Jacobi(pDeviceContext);
	return pSolver->Init();
}

HRESULT Jacobi::Init()
{
	HRESULT hr;
	ID3DBlob *shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"CSJacobi.cso", &shaderBuffer));

	hr = m_pd3dDevice->CreateComputeShader(shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), nullptr, &m_pShader);

	if (SUCCEEDED(hr))
	{
		ID3D11ShaderReflection *pReflector = nullptr;
		auto h = D3DReflect(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
			IID_ID3D11ShaderReflection, (void**)&pReflector);

		if (SUCCEEDED(h))
		{
			D3D11_SHADER_INPUT_BIND_DESC desc;
			h = pReflector->GetResourceBindingDescByName("b", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("x", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot = desc.BindPoint;
			else hr = h;
		}
		else hr = h;

		if (pReflector) pReflector->Release();
	}

	if (shaderBuffer) shaderBuffer->Release();

	return hr;
}

void Jacobi::Solve(const XMUINT3 &vSize, ID3D11ShaderResourceView *const pSrc, ID3D11UnorderedAccessView *const pDst, uint32_t uNumIt)
{
	const auto UAVInitialCounts = 0u;

	// Setup
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot, 1, &pSrc);

	// Jacobi iterations
	for (auto i = 0u; i < uNumIt; ++i) jacobi(vSize, pDst);

	// Unset
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot, 1, &g_pNullSRV);
}

void Jacobi::AddRef()
{
	++m_uRefCount;
}

void Jacobi::Release()
{
	if (--m_uRefCount < 1) delete this;
}

void Jacobi::jacobi(const XMUINT3 &vSize, ID3D11UnorderedAccessView *const pDst)
{
	const auto UAVInitialCounts = 0u;

	// Setup
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot, 1, &pDst, &UAVInitialCounts);

	// Jacobi iteration
	m_pd3dContext->CSSetShader(m_pShader, nullptr, 0);
	m_pd3dContext->Dispatch(vSize.x / THREAD_GROUP_SIZE, vSize.y / THREAD_GROUP_SIZE, vSize.z / THREAD_GROUP_SIZE);
}
