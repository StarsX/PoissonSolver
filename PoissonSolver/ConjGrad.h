#pragma once

#ifdef PRECOMPILED_HEADER
#include PRECOMPILED_HEADER_FILE
#else
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
//#include <d3dcsx.h>
#endif

#include <fstream>
#include <vector>
#include "PrefixSum.h"

class ConjGrad
{
public:
	ConjGrad(ID3D11DeviceContext *const pDeviceContext);
	virtual ~ConjGrad();
	HRESULT Init(DXGI_FORMAT eFormat, const DirectX::XMUINT3 &vSize,
		ID3D11ComputeShader *const pInitShader = nullptr, ID3D11ComputeShader *const pApShader = nullptr);

	void Solve(const DirectX::XMUINT3 &vSize, ID3D11ShaderResourceView *const pSrc, ID3D11UnorderedAccessView *const pDst, uint32_t iNumIt);
	void AddRef();
	void Release();

	static HRESULT CreateSolver(ID3D11DeviceContext *const pDeviceContext,
		DXGI_FORMAT eFormat, const DirectX::XMUINT3 &vSize, ConjGrad **ppSolver,
		ID3D11ComputeShader *const pInitShader = nullptr, ID3D11ComputeShader *const pApShader = nullptr);
protected:
	HRESULT initShaders(ID3D11ComputeShader *const pInitShader, ID3D11ComputeShader *const pApShader);
	HRESULT initBuffers(DXGI_FORMAT eFormat, const DirectX::XMUINT3 &vSize);
	HRESULT createConstBuffer(const DirectX::XMUINT3 &vSize);
	void init(const DirectX::XMUINT3 &vSize, ID3D11ShaderResourceView *const pSrc, ID3D11UnorderedAccessView *const pDst);
	void update_x(const DirectX::XMUINT3 &vSize, ID3D11UnorderedAccessView *const pDst);
	void update_p(const DirectX::XMUINT3 &vSize);
	void compute_pAp(const DirectX::XMUINT3 &vSize);
	void swapBuffers();
	void computeElementSize(DXGI_FORMAT eFormat);

	D3DX11_SCAN_DATA_TYPE		m_eScanDataType;

	uint32_t					m_uElementSize;

	uint32_t					m_uRefCount;
	uint32_t					m_uSRVSlot_b;
	uint32_t					m_uSRVSlot_r;
	uint32_t					m_uSRVSlot_Acc_rr;
	uint32_t					m_uSRVSlot_Acc_rr_prev;
	uint32_t					m_uSRVSlot_Acc_rr_new;
	uint32_t					m_uSRVSlot_p;
	uint32_t					m_uSRVSlot_p_new;
	uint32_t					m_uSRVSlot_Ap;
	uint32_t					m_uSRVSlot_Acc_pAp;

	uint32_t					m_uUAVSlot_x0;
	uint32_t					m_uUAVSlot_x;
	uint32_t					m_uUAVSlot_r0;
	uint32_t					m_uUAVSlot_r;
	uint32_t					m_uUAVSlot_rr0;
	uint32_t					m_uUAVSlot_rr;
	uint32_t					m_uUAVSlot_p0;
	uint32_t					m_uUAVSlot_p;
	uint32_t					m_uUAVSlot_Ap;
	uint32_t					m_uUAVSlot_pAp;

	uint32_t					m_uCBSlot_init;
	uint32_t					m_uCBSlot_x;
	uint32_t					m_uCBSlot_p;
	uint32_t					m_uCBSlot_pAp;

	ID3D11Buffer				*m_pCBDim;

	ID3D11Buffer				*m_prr[2];
	ID3D11Buffer				*m_pr;

	ID3D11Buffer				*m_pAp;
	ID3D11Buffer				*m_ppAp;

	ID3D11Texture3D				*m_pp;

	ID3D11ShaderResourceView	*m_pSRVr;
	ID3D11ShaderResourceView	*m_pSRVAcc_rr;
	ID3D11ShaderResourceView	*m_pSRVAcc_rr_new;
	ID3D11ShaderResourceView	*m_pSRVp;
	ID3D11ShaderResourceView	*m_pSRVAp;
	ID3D11ShaderResourceView	*m_pSRVAcc_pAp;

	ID3D11UnorderedAccessView	*m_pUAVr;
	ID3D11UnorderedAccessView	*m_pUAVrr;
	ID3D11UnorderedAccessView	*m_pUAVrr0;
	ID3D11UnorderedAccessView	*m_pUAVp;
	ID3D11UnorderedAccessView	*m_pUAVAp;
	ID3D11UnorderedAccessView	*m_pUAVpAp;

	ID3D11ComputeShader			*m_pInitShader;
	ID3D11ComputeShader			*m_pShader;
	ID3D11ComputeShader			*m_pUpdateShader;
	ID3D11ComputeShader			*m_pApShader;

	//ID3DX11Scan					*m_pScan;
	PrefixSum					*m_pScan;

	ID3D11Device				*m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dContext;
};
