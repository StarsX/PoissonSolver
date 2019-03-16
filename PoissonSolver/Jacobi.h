//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#pragma once

#ifdef PRECOMPILED_HEADER
#include PRECOMPILED_HEADER_FILE
#else
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#endif

#include <fstream>
#include <vector>

class Jacobi
{
public:
	Jacobi(ID3D11DeviceContext *const pDeviceContext);
	virtual ~Jacobi();
	HRESULT Init();

	void Solve(const DirectX::XMUINT3 &vSize, ID3D11UnorderedAccessView *const pSrc, ID3D11UnorderedAccessView *const pDst, uint32_t uNumIt);
	void AddRef();
	void Release();

	static HRESULT CreateSolver(ID3D11DeviceContext *const pDeviceContext, Jacobi **ppSolver);
protected:
	void jacobi(const DirectX::XMUINT3 &vSize, ID3D11UnorderedAccessView *const pDst);

	uint32_t			m_uRefCount;
	uint32_t			m_uUAVSlot_b;
	uint32_t			m_uUAVSlot_x;

	ID3D11ComputeShader	*m_pShader;

	ID3D11Device		*m_pd3dDevice;
	ID3D11DeviceContext	*m_pd3dContext;
};
