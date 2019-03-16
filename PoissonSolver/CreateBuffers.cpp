//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "CreateBuffers.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Create Structured Buffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateTypedBuffer(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, void* pInitData, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	const auto desc = CD3D11_BUFFER_DESC(uElementSize * uCount, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);

	if (pInitData)
	{
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufOut);
	}
	else return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Structured Buffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, void* pInitData, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = uElementSize * uCount;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = uElementSize;

	if (pInitData)
	{
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufOut);
	}
	else
		return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Raw Buffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateRawBuffer(ID3D11Device* pDevice, UINT uSize, void* pInitData, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = uSize;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	if (pInitData)
	{
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &initData, ppBufOut);
	}
	else
		return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Texture 3D
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateTexture3D(ID3D11Device* pDevice, DXGI_FORMAT eFormat, XMUINT3 vSize, void* pInitData, ID3D11Texture3D** ppTexOut, uint32_t uMips)
{
	*ppTexOut = nullptr;

	auto desc = CD3D11_TEXTURE3D_DESC(eFormat, vSize.x, vSize.y, vSize.z, uMips);
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	if (pInitData && uMips == 1)
	{
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = pInitData;
		initData.SysMemPitch = sizeof(float) * vSize.x;
		initData.SysMemSlicePitch = sizeof(float) * vSize.x * vSize.y;
		return pDevice->CreateTexture3D(&desc, &initData, ppTexOut);
	}
	else
	{
		const auto hr = pDevice->CreateTexture3D(&desc, nullptr, ppTexOut);
		if (SUCCEEDED(hr) && pInitData)
		{
			ID3D11DeviceContext* pContext;
			pDevice->GetImmediateContext(&pContext);
			if (pContext)
			{
				pContext->UpdateSubresource(*ppTexOut, 0, nullptr, pInitData, sizeof(float) * vSize.x, sizeof(float) * vSize.x * vSize.y);
				pContext->Release();
			}
		}

		return hr;
	}
}

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Structured or Raw Buffers
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateBufferSRV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut, DXGI_FORMAT eFormat)
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pBuffer->GetDesc(&descBuf);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	desc.BufferEx.FirstElement = 0;

	if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
	{
		// This is a Raw Buffer

		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
	}
	else if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
	{
		// This is a Structured Buffer

		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
	}
	else if (eFormat != DXGI_FORMAT_UNKNOWN)
	{
		desc.Format = eFormat;
		desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
	}
	else return E_INVALIDARG;

	return pDevice->CreateShaderResourceView(pBuffer, &desc, ppSRVOut);
}

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Texture3D
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateTexture3DSRV(ID3D11Device* pDevice, ID3D11Texture3D* pTex, ID3D11ShaderResourceView** ppSRVOut, uint32_t uMDMip, uint32_t uMips)
{
	// Setup the description of the shader resource view.
	const auto desc = CD3D11_SHADER_RESOURCE_VIEW_DESC(pTex, DXGI_FORMAT_UNKNOWN, uMDMip, uMips);
	// Create the shader resource view.
	return pDevice->CreateShaderResourceView(pTex, &desc, ppSRVOut);
}

//--------------------------------------------------------------------------------------
// Create Unordered Access View for Structured or Raw Buffers
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
HRESULT CreateBufferUAV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut, DXGI_FORMAT eFormat)
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pBuffer->GetDesc(&descBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;

	if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
	{
		// This is a Raw Buffer

		desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
		desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		desc.Buffer.NumElements = descBuf.ByteWidth / 4;
	}
	else if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
	{
		// This is a Structured Buffer

		desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
		desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
	}
	else if (eFormat != DXGI_FORMAT_UNKNOWN)
	{
		desc.Format = eFormat;
		desc.Buffer.NumElements = descBuf.ByteWidth / 4;
	}
	else return E_INVALIDARG;

	return pDevice->CreateUnorderedAccessView(pBuffer, &desc, ppUAVOut);
}

//--------------------------------------------------------------------------------------
// Create Unordered Access View for Texture3D
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
HRESULT CreateTexture3DUAV(ID3D11Device* pDevice, ID3D11Texture3D* pTex, ID3D11UnorderedAccessView** ppUAVOut, uint32_t uMipSlice)
{
	// Setup the description of the shader resource view.
	const auto desc = CD3D11_UNORDERED_ACCESS_VIEW_DESC(pTex, DXGI_FORMAT_UNKNOWN, uMipSlice);
	// Create the shader resource view.
	return pDevice->CreateUnorderedAccessView(pTex, &desc, ppUAVOut);
}
