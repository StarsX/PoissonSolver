#include "ConjGrad.h"
#include "SharedConst.h"

#ifndef V_RETURN
#define V_RETURN(x)	{ hr = x; if (FAILED(hr)) return hr; }
#endif

using namespace DirectX;
using namespace std;

ID3D11ShaderResourceView	*const	g_pNullSRV = nullptr;	// Helper to Clear SRVs
ID3D11UnorderedAccessView	*const	g_pNullUAV = nullptr;	// Helper to Clear UAVs
ID3D11Buffer				*const	g_pNullBuffer = nullptr;

ConjGrad::ConjGrad(ID3D11DeviceContext *pDeviceContext)
	: m_pd3dContext(pDeviceContext), m_uRefCount(1)
{
	m_pd3dContext->AddRef();
	m_pd3dContext->GetDevice(&m_pd3dDevice);
}

ConjGrad::~ConjGrad()
{
	{
		if (m_pUAVr) m_pUAVr->Release();
		if (m_pUAVrr) m_pUAVrr->Release();
		if (m_pUAVrr0) m_pUAVrr0->Release();

		if (m_pUAVp) m_pUAVp->Release();
		if (m_pUAVAp) m_pUAVAp->Release();
		if (m_pUAVpAp) m_pUAVpAp->Release();
	}

	{
		if (m_pSRVr) m_pSRVr->Release();
		if (m_pSRVAcc_rr) m_pSRVAcc_rr->Release();
		if (m_pSRVAcc_rr_new) m_pSRVAcc_rr_new->Release();

		if (m_pSRVp) m_pSRVp->Release();
		if (m_pSRVAp) m_pSRVAp->Release();
		if (m_pSRVAcc_pAp) m_pSRVAcc_pAp->Release();
	}

	{
		if (m_pr) m_pr->Release();
		if (m_prr[0]) m_prr[0]->Release();
		if (m_prr[1]) m_prr[1]->Release();

		if (m_pp) m_pp->Release();
		if (m_pAp) m_pAp->Release();
		if (m_ppAp) m_ppAp->Release();
	}

	if (m_pInitShader) m_pInitShader->Release();
	if (m_pShader) m_pShader->Release();
	if (m_pUpdateShader) m_pUpdateShader->Release();
	if (m_pApShader) m_pApShader->Release();

	if (m_pScan) m_pScan->Release();

	if (m_pd3dContext) m_pd3dContext->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
}

HRESULT ConjGrad::CreateSolver(ID3D11DeviceContext *const pDeviceContext,
	DXGI_FORMAT eFormat, const XMUINT3 &vSize, ConjGrad **ppSolver,
	ID3D11ComputeShader *const pInitShader, ID3D11ComputeShader *const pApShader)
{
	auto &pSolver = *ppSolver;
	pSolver = new ConjGrad(pDeviceContext);
	return pSolver->Init(eFormat, vSize, pInitShader, pApShader);
}

HRESULT ConjGrad::Init(DXGI_FORMAT eFormat, const XMUINT3 &vSize,
	ID3D11ComputeShader *const pInitShader, ID3D11ComputeShader *const pApShader)
{
	HRESULT hr;
	computeElementSize(eFormat);

	V_RETURN(initShaders(pInitShader, pApShader));
	V_RETURN(initBuffers(eFormat, vSize));
	V_RETURN(createConstBuffer(vSize));

	V_RETURN(D3DX11CreateScan(m_pd3dContext, vSize.x * vSize.y * vSize.z + 1, 1, &m_pScan));
	V_RETURN(m_pScan->SetScanDirection(D3DX11_SCAN_DIRECTION_FORWARD));

	return hr;
}

HRESULT ConjGrad::initBuffers(DXGI_FORMAT eFormat, const XMUINT3 &vSize)
{
	HRESULT hr;

	const auto uSize = vSize.x * vSize.y * vSize.z;
	const auto uScanSize = uSize + 1;

	// r related
	V_RETURN(CreateTypedBuffer(m_pd3dDevice, m_uElementSize, uScanSize, nullptr, &m_prr[0]));
	V_RETURN(CreateBufferSRV(m_pd3dDevice, m_prr[0], &m_pSRVAcc_rr, eFormat));
	V_RETURN(CreateBufferUAV(m_pd3dDevice, m_prr[0], &m_pUAVrr0, eFormat));

	V_RETURN(CreateTypedBuffer(m_pd3dDevice, m_uElementSize, uScanSize, nullptr, &m_prr[1]));
	V_RETURN(CreateBufferSRV(m_pd3dDevice, m_prr[1], &m_pSRVAcc_rr_new, eFormat));
	V_RETURN(CreateBufferUAV(m_pd3dDevice, m_prr[1], &m_pUAVrr, eFormat));

	V_RETURN(CreateTypedBuffer(m_pd3dDevice, m_uElementSize, uSize, nullptr, &m_pr));
	V_RETURN(CreateBufferSRV(m_pd3dDevice, m_pr, &m_pSRVr, eFormat));
	V_RETURN(CreateBufferUAV(m_pd3dDevice, m_pr, &m_pUAVr, eFormat));

	// p related
	V_RETURN(CreateTypedBuffer(m_pd3dDevice, m_uElementSize, uSize, nullptr, &m_pAp));
	V_RETURN(CreateBufferSRV(m_pd3dDevice, m_pAp, &m_pSRVAp, eFormat));
	V_RETURN(CreateBufferUAV(m_pd3dDevice, m_pAp, &m_pUAVAp, eFormat));
	
	V_RETURN(CreateTypedBuffer(m_pd3dDevice, m_uElementSize, uScanSize, nullptr, &m_ppAp));
	V_RETURN(CreateBufferSRV(m_pd3dDevice, m_ppAp, &m_pSRVAcc_pAp, eFormat));
	V_RETURN(CreateBufferUAV(m_pd3dDevice, m_ppAp, &m_pUAVpAp, eFormat));

	V_RETURN(CreateTexture3D(m_pd3dDevice, eFormat, vSize, nullptr, &m_pp));
	V_RETURN(CreateTexture3DSRV(m_pd3dDevice, m_pp, &m_pSRVp));
	V_RETURN(CreateTexture3DUAV(m_pd3dDevice, m_pp, &m_pUAVp));

	return hr;
}

HRESULT ConjGrad::initShaders(ID3D11ComputeShader *const pInitShader, ID3D11ComputeShader *const pApShader)
{
	HRESULT h, hr;

	ID3DBlob *shaderBuffer = nullptr;
	D3D11_SHADER_INPUT_BIND_DESC desc;

	if (pInitShader) {
		m_pInitShader = pInitShader;
		m_pInitShader->AddRef();
	}
	else {
		V_RETURN(D3DReadFileToBlob(L"InitCS.cso", &shaderBuffer));
		hr = m_pd3dDevice->CreateComputeShader(
			shaderBuffer->GetBufferPointer(),
			shaderBuffer->GetBufferSize(),
			nullptr, &m_pInitShader
			);
		if (SUCCEEDED(hr)) {
			ID3D11ShaderReflection *pReflector = nullptr;
			hr = D3DReflect(
				shaderBuffer->GetBufferPointer(),
				shaderBuffer->GetBufferSize(),
				IID_ID3D11ShaderReflection,
				(void**)&pReflector
				);
			if (SUCCEEDED(hr)) {
				h = pReflector->GetResourceBindingDescByName("b", &desc);
				if (SUCCEEDED(h)) m_uSRVSlot_b = desc.BindPoint;
				else hr = h;

				h = pReflector->GetResourceBindingDescByName("x", &desc);
				if (SUCCEEDED(h)) m_uUAVSlot_x0 = desc.BindPoint;
				else hr = h;
				h = pReflector->GetResourceBindingDescByName("r", &desc);
				if (SUCCEEDED(h)) m_uUAVSlot_r0 = desc.BindPoint;
				else hr = h;
				h = pReflector->GetResourceBindingDescByName("rr", &desc);
				if (SUCCEEDED(h)) m_uUAVSlot_rr0 = desc.BindPoint;
				else hr = h;
				h = pReflector->GetResourceBindingDescByName("p", &desc);
				if (SUCCEEDED(h)) m_uUAVSlot_p0 = desc.BindPoint;
				else hr = h;

				h = pReflector->GetResourceBindingDescByName("cbDimension", &desc);
				if (SUCCEEDED(h)) m_uCBSlot_init = desc.BindPoint;
				else hr = h;
			}
			if (pReflector) pReflector->Release();
		}
		if (shaderBuffer) shaderBuffer->Release();
		if (FAILED(hr)) return hr;
	}

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"xUpdateCS.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pShader
		);
	if (SUCCEEDED(hr)) {
		ID3D11ShaderReflection *pReflector = nullptr;
		hr = D3DReflect(
			shaderBuffer->GetBufferPointer(),
			shaderBuffer->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			(void**)&pReflector
			);
		if (SUCCEEDED(hr)) {
			h = pReflector->GetResourceBindingDescByName("p_RO", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot_p = desc.BindPoint;
			else hr = h;
			h = pReflector->GetResourceBindingDescByName("Ap", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot_Ap = desc.BindPoint;
			else hr = h;
			h = pReflector->GetResourceBindingDescByName("Acc_rr", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot_Acc_rr = desc.BindPoint;
			else hr = h;
			h = pReflector->GetResourceBindingDescByName("Acc_pAp", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot_Acc_pAp = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("x", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_x = desc.BindPoint;
			else hr = h;
			h = pReflector->GetResourceBindingDescByName("r", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_r = desc.BindPoint;
			else hr = h;
			h = pReflector->GetResourceBindingDescByName("rr", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_rr = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("cbDimension", &desc);
			if (SUCCEEDED(h)) m_uCBSlot_x = desc.BindPoint;
			else hr = h;
		}
		if (pReflector) pReflector->Release();
	}
	if (shaderBuffer) shaderBuffer->Release();
	if (FAILED(hr)) return hr;

	shaderBuffer = nullptr;
	V_RETURN(D3DReadFileToBlob(L"pUpdateCS.cso", &shaderBuffer));
	hr = m_pd3dDevice->CreateComputeShader(
		shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(),
		nullptr, &m_pUpdateShader
		);
	if (SUCCEEDED(hr)) {
		ID3D11ShaderReflection *pReflector = nullptr;
		hr = D3DReflect(
			shaderBuffer->GetBufferPointer(),
			shaderBuffer->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			(void**)&pReflector
			);
		if (SUCCEEDED(hr)) {
			h = pReflector->GetResourceBindingDescByName("r_RO", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot_r = desc.BindPoint;
			else hr = h;
			h = pReflector->GetResourceBindingDescByName("Acc_rr", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot_Acc_rr_prev = desc.BindPoint;
			else hr = h;
			h = pReflector->GetResourceBindingDescByName("Acc_rr_new", &desc);
			if (SUCCEEDED(h)) m_uSRVSlot_Acc_rr_new = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("p", &desc);
			if (SUCCEEDED(h)) m_uUAVSlot_p = desc.BindPoint;
			else hr = h;

			h = pReflector->GetResourceBindingDescByName("cbDimension", &desc);
			if (SUCCEEDED(h)) m_uCBSlot_p = desc.BindPoint;
			else hr = h;
		}
		if (pReflector) pReflector->Release();
	}
	if (shaderBuffer) shaderBuffer->Release();
	if (FAILED(hr)) return hr;

	if (pApShader) {
		m_pApShader = pApShader;
		m_pApShader->AddRef();
	}
	else {
		V_RETURN(D3DReadFileToBlob(L"pApCS.cso", &shaderBuffer));
		hr = m_pd3dDevice->CreateComputeShader(
			shaderBuffer->GetBufferPointer(),
			shaderBuffer->GetBufferSize(),
			nullptr, &m_pApShader
			);
		if (SUCCEEDED(hr)) {
			ID3D11ShaderReflection *pReflector = nullptr;
			hr = D3DReflect(
				shaderBuffer->GetBufferPointer(),
				shaderBuffer->GetBufferSize(),
				IID_ID3D11ShaderReflection,
				(void**)&pReflector
				);
			if (SUCCEEDED(hr)) {
				h = pReflector->GetResourceBindingDescByName("x", &desc);
				if (SUCCEEDED(h)) m_uSRVSlot_p_new = desc.BindPoint;
				else hr = h;

				h = pReflector->GetResourceBindingDescByName("Ap", &desc);
				if (SUCCEEDED(h)) m_uUAVSlot_Ap = desc.BindPoint;
				else hr = h;
				h = pReflector->GetResourceBindingDescByName("pAp", &desc);
				if (SUCCEEDED(h)) m_uUAVSlot_pAp = desc.BindPoint;
				else hr = h;

				h = pReflector->GetResourceBindingDescByName("cbDimension", &desc);
				if (SUCCEEDED(h)) m_uCBSlot_pAp = desc.BindPoint;
				else hr = h;
			}
			if (pReflector) pReflector->Release();
		}
		if (shaderBuffer) shaderBuffer->Release();
		if (FAILED(hr)) return hr;
	}

	return hr;
}

HRESULT ConjGrad::createConstBuffer(const XMUINT3 &vSize)
{
	D3D11_BUFFER_DESC desc = CD3D11_BUFFER_DESC(sizeof(XMUINT4), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_IMMUTABLE);
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &vSize;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;
	return m_pd3dDevice->CreateBuffer(&desc, &initData, &m_pCBDim);
}

void ConjGrad::Solve(const XMUINT3 &vSize, ID3D11ShaderResourceView *const pSrc, ID3D11UnorderedAccessView *const pDst, uint32_t iNumIt)
{
	const auto UAVInitialCounts = 0u;
	const auto uScanSize = vSize.x * vSize.y * vSize.z + 1;
	
	// Initial solution
	init(vSize, pSrc, pDst);
	m_pScan->Scan(m_eScanDataType, D3DX11_SCAN_OPCODE_ADD, uScanSize, m_pUAVrr0, m_pUAVrr0);
	m_pd3dContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);

	// Iteration
	for (auto i = 0u; i < iNumIt; ++i) {
		compute_pAp(vSize);
		m_pScan->Scan(m_eScanDataType, D3DX11_SCAN_OPCODE_ADD, uScanSize, m_pUAVpAp, m_pUAVpAp);
		m_pd3dContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
		update_x(vSize, pDst);

		m_pScan->Scan(m_eScanDataType, D3DX11_SCAN_OPCODE_ADD, uScanSize, m_pUAVrr, m_pUAVrr);
		m_pd3dContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
		update_p(vSize);
		
		swapBuffers();
	}
}

void ConjGrad::AddRef()
{
	++m_uRefCount;
}

void ConjGrad::Release()
{
	if (--m_uRefCount < 1) delete this;
}

void ConjGrad::init(const XMUINT3 &vSize, ID3D11ShaderResourceView *const pSrc, ID3D11UnorderedAccessView *const pDst)
{
	const auto UAVInitialCounts = 0u;

	// Setup
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_init, 1, &m_pCBDim);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_b, 1, &pSrc);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_x0, 1, &pDst, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_r0, 1, &m_pUAVr, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_rr0, 1, &m_pUAVrr0, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_p0, 1, &m_pUAVp, &UAVInitialCounts);

	// initial solution
	m_pd3dContext->CSSetShader(m_pInitShader, nullptr, 0);
	m_pd3dContext->Dispatch(vSize.x, vSize.y, vSize.z);

	// Unset
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_p0, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_rr0, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_r0, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_x0, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_b, 1, &g_pNullSRV);
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_init, 1, &g_pNullBuffer);
}

void ConjGrad::update_x(const XMUINT3 &vSize, ID3D11UnorderedAccessView *const pDst)
{
	const auto UAVInitialCounts = 0u;

	// Setup
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_x, 1, &m_pCBDim);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_p, 1, &m_pSRVp);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Ap, 1, &m_pSRVAp);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_rr, 1, &m_pSRVAcc_rr);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_pAp, 1, &m_pSRVAcc_pAp);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_x, 1, &pDst, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_r, 1, &m_pUAVr, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_rr, 1, &m_pUAVrr, &UAVInitialCounts);

	// update solution
	m_pd3dContext->CSSetShader(m_pShader, nullptr, 0);
	m_pd3dContext->Dispatch(vSize.x / THREAD_GROUP_SIZE, vSize.y / THREAD_GROUP_SIZE, vSize.z / THREAD_GROUP_SIZE);

	// Unset
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_rr, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_r, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_x, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_pAp, 1, &g_pNullSRV);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_rr, 1, &g_pNullSRV);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Ap, 1, &g_pNullSRV);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_p, 1, &g_pNullSRV);
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_x, 1, &g_pNullBuffer);
}

void ConjGrad::update_p(const XMUINT3 &vSize)
{
	const auto UAVInitialCounts = 0u;

	// Setup
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_p, 1, &m_pCBDim);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_r, 1, &m_pSRVr);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_rr_prev, 1, &m_pSRVAcc_rr);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_rr_new, 1, &m_pSRVAcc_rr_new);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_p, 1, &m_pUAVp, &UAVInitialCounts);

	// update solution
	m_pd3dContext->CSSetShader(m_pUpdateShader, nullptr, 0);
	m_pd3dContext->Dispatch(vSize.x / THREAD_GROUP_SIZE, vSize.y / THREAD_GROUP_SIZE, vSize.z / THREAD_GROUP_SIZE);

	// Unset
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_p, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_rr_new, 1, &g_pNullSRV);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_Acc_rr_prev, 1, &g_pNullSRV);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_r, 1, &g_pNullSRV);
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_p, 1, &g_pNullBuffer);
}

void ConjGrad::compute_pAp(const XMUINT3 &vSize)
{
	const auto UAVInitialCounts = 0u;

	// Setup
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_pAp, 1, &m_pCBDim);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_p_new, 1, &m_pSRVp);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Ap, 1, &m_pUAVAp, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_pAp, 1, &m_pUAVpAp, &UAVInitialCounts);

	// update solution
	m_pd3dContext->CSSetShader(m_pApShader, nullptr, 0);
	m_pd3dContext->Dispatch(vSize.x / THREAD_GROUP_SIZE, vSize.y / THREAD_GROUP_SIZE, vSize.z / THREAD_GROUP_SIZE);

	// Unset
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_pAp, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetUnorderedAccessViews(m_uUAVSlot_Ap, 1, &g_pNullUAV, &UAVInitialCounts);
	m_pd3dContext->CSSetShaderResources(m_uSRVSlot_p_new, 1, &g_pNullSRV);
	m_pd3dContext->CSSetConstantBuffers(m_uCBSlot_pAp, 1, &g_pNullBuffer);
}

void ConjGrad::swapBuffers()
{
	ID3D11ShaderResourceView* pSRV = m_pSRVAcc_rr_new;
	m_pSRVAcc_rr_new = m_pSRVAcc_rr;
	m_pSRVAcc_rr = pSRV;

	ID3D11UnorderedAccessView* pUAV = m_pUAVrr0;
	m_pUAVrr0 = m_pUAVrr;
	m_pUAVrr = pUAV;
}

void ConjGrad::computeElementSize(DXGI_FORMAT eFormat)
{
	switch (eFormat)
	{
		case DXGI_FORMAT_R32_FLOAT:
			m_uElementSize = sizeof(float);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_FLOAT;
			return;
		case DXGI_FORMAT_R32_UINT:
			m_uElementSize = sizeof(uint32_t);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_UINT;
			return;
		case DXGI_FORMAT_R32_SINT:
			m_uElementSize = sizeof(int32_t);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_INT;
			return;

		case DXGI_FORMAT_R32G32_FLOAT:
			m_uElementSize = sizeof(XMFLOAT2);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_FLOAT;
			return;
		case DXGI_FORMAT_R32G32_UINT:
			m_uElementSize = sizeof(XMUINT2);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_UINT;
			return;
		case DXGI_FORMAT_R32G32_SINT:
			m_uElementSize = sizeof(XMINT2);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_INT;
			return;

		case DXGI_FORMAT_R32G32B32_FLOAT:
			m_uElementSize = sizeof(XMFLOAT3);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_FLOAT;
			return;
		case DXGI_FORMAT_R32G32B32_UINT:
			m_uElementSize = sizeof(XMUINT3);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_UINT;
			return;
		case DXGI_FORMAT_R32G32B32_SINT:
			m_uElementSize = sizeof(XMINT3);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_INT;
			return;

		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			m_uElementSize = sizeof(XMFLOAT4);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_FLOAT;
			return;
		case DXGI_FORMAT_R32G32B32A32_UINT:
			m_uElementSize = sizeof(XMUINT4);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_UINT;
			return;
		case DXGI_FORMAT_R32G32B32A32_SINT:
			m_uElementSize = sizeof(XMINT4);
			m_eScanDataType = D3DX11_SCAN_DATA_TYPE_INT;
			return;
	}
}
