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
#include <d3dcsx.h>
#endif

#include "CreateBuffers.h"

class PrefixSum
{
public:
	enum SCAN_DATA_TYPE
	{
		SCAN_DATA_TYPE_FLOAT = 1,
		SCAN_DATA_TYPE_INT,
		SCAN_DATA_TYPE_UINT
	};

	PrefixSum(ID3D11DeviceContext *const pDeviceContext);
	virtual ~PrefixSum();

	HRESULT Init(const uint32_t uSize);

	void Scan(const SCAN_DATA_TYPE dataType, const uint32_t uSize, ID3D11UnorderedAccessView *const pUAVSrc, ID3D11UnorderedAccessView *const pUAVDst);
	void AddRef();
	void Release();

	static HRESULT CreateScan(ID3D11DeviceContext *const pDeviceContext, const uint32_t uSize, PrefixSum **ppScan);

protected:
	enum ComputeShaderID : uint32_t
	{
		CS_PREFIXSUM1_FLOAT,
		CS_PREFIXSUM1_FLOAT_RW,
		CS_PREFIXSUM2_FLOAT,
		CS_PREFIXSUM1_INT,
		CS_PREFIXSUM1_INT_RW,
		CS_PREFIXSUM2_INT,
		CS_PREFIXSUM1_UINT,
		CS_PREFIXSUM1_UINT_RW,
		CS_PREFIXSUM2_UINT,

		NUM_CS
	};

	uint32_t					m_uRefCount;

	uint32_t					m_uUAVSlot_Dst;
	uint32_t					m_uUAVSlot_Inc;
	uint32_t					m_uUAVSlot_Src;

	ID3D11Buffer				*m_pInc;
	ID3D11UnorderedAccessView	*m_pUAVInc;

	ID3D11ComputeShader			*m_pShaders[NUM_CS];

	ID3D11Device				*m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dContext;
};
