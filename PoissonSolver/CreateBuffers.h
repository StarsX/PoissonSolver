#pragma once

#ifdef PRECOMPILED_HEADER
#include PRECOMPILED_HEADER_FILE
#else
#include <d3d11.h>
#include <DirectXMath.h>
#endif

HRESULT CreateTypedBuffer(_In_ ID3D11Device* pDevice, _In_ UINT uElementSize, _In_ UINT uCount,
	_In_reads_(uElementSize*uCount) void* pInitData, _Outptr_ ID3D11Buffer** ppBufOut);
HRESULT CreateStructuredBuffer(_In_ ID3D11Device* pDevice, _In_ UINT uElementSize, _In_ UINT uCount,
	_In_reads_(uElementSize*uCount) void* pInitData, _Outptr_ ID3D11Buffer** ppBufOut);
HRESULT CreateRawBuffer(_In_ ID3D11Device* pDevice, _In_ UINT uSize, _In_reads_(uSize) void* pInitData, _Outptr_ ID3D11Buffer** ppBufOut);
HRESULT CreateTexture3D(_In_ ID3D11Device* pDevice, _In_ DXGI_FORMAT eFormat, _In_ DirectX::XMUINT3 vSize,
	_In_ void* pInitData, _Outptr_ ID3D11Texture3D** ppTexOut);
HRESULT CreateBufferSRV(_In_ ID3D11Device* pDevice, _In_ ID3D11Buffer* pBuffer, _Outptr_ ID3D11ShaderResourceView** ppSRVOut, DXGI_FORMAT eFormat = DXGI_FORMAT_UNKNOWN);
HRESULT CreateTexture3DSRV(_In_ ID3D11Device* pDevice, _In_ ID3D11Texture3D* pTex, _Outptr_ ID3D11ShaderResourceView** ppSRVOut);
HRESULT CreateBufferUAV(_In_ ID3D11Device* pDevice, _In_ ID3D11Buffer* pBuffer, _Outptr_ ID3D11UnorderedAccessView** pUAVOut, DXGI_FORMAT eFormat = DXGI_FORMAT_UNKNOWN);
HRESULT CreateTexture3DUAV(_In_ ID3D11Device* pDevice, _In_ ID3D11Texture3D* pTex, _Outptr_ ID3D11UnorderedAccessView** ppUAVOut);
