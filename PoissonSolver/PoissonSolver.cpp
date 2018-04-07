//--------------------------------------------------------------------------------------
// File: BasicCompute11.cpp
//
// Demonstrates the basics to get DirectX 11 Compute Shader (aka DirectCompute) up and
// running by implementing Array A + Array B
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <stdio.h>
#include <crtdbg.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <d3dcompiler.h>
//#include <array>

#include "CreateBuffers.h"
#include "Jacobi.h"
#include "ConjGrad.h"

using namespace DirectX;
//using namespace std;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcsx.lib")

#if D3D_COMPILER_VERSION < 46
#include <d3dx11.h>
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=nullptr; } }
#endif

// Comment out the following line to use raw buffers instead of structured buffers
#define USE_STRUCTURED_BUFFERS

// If defined, then the hardware/driver must report support for double-precision CS 5.0 shaders or the sample fails to run
//#define TEST_DOUBLE

#if defined(_MSC_VER) && (_MSC_VER<1610) && !defined(_In_reads_)
#define _Outptr_
#define _Outptr_opt_ 
#define _In_reads_(exp)
#define _In_reads_opt_(exp)
#define _Out_writes_(exp)
#endif

#ifndef _Use_decl_annotations_
#define _Use_decl_annotations_
#endif

#define ITERATIONS	64

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
HRESULT CreateComputeDevice(_Outptr_ ID3D11Device** ppDeviceOut, _Outptr_ ID3D11DeviceContext** ppContextOut, _In_ bool bForceRef);
ID3D11Texture3D* CreateAndCopyToDebugTex(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext, _In_ ID3D11Texture3D* pTex);
HRESULT FindDXSDKShaderFileCch(_Out_writes_(cchDest) WCHAR* strDestPath,
	_In_ int cchDest,
	_In_z_ LPCWSTR strFilename);

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3D11Device*               g_pDevice = nullptr;
ID3D11DeviceContext*        g_pContext = nullptr;

Jacobi*						g_pSolverJacobi = nullptr;
ID3D11Texture3D*			g_px = nullptr;
ID3D11Texture3D*			g_pb = nullptr;
ID3D11ShaderResourceView*   g_pbSRV = nullptr;
ID3D11UnorderedAccessView*  g_pxUAV = nullptr;

ConjGrad*					g_pSolverConjGrad = nullptr;
ID3D11Texture3D*			g_px_CG = nullptr;
ID3D11UnorderedAccessView*  g_pxUAV_CG = nullptr;

//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int __cdecl main()
{
	// Enable run-time memory check for debug builds.
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif    

	printf("Creating device...");
	if (FAILED(CreateComputeDevice(&g_pDevice, &g_pContext, false)))
		return 1;
	printf("done\n");

	const auto uDim = 16u;
	const XMUINT3 vSize(uDim, uDim, uDim);
	float *b = new float[vSize.x * vSize.y * vSize.z];

	printf("Creating solvers...");
	if (FAILED(Jacobi::CreateSolver(g_pContext, &g_pSolverJacobi))) return 1;
	if (FAILED(ConjGrad::CreateSolver(g_pContext, DXGI_FORMAT_R32_FLOAT, vSize, &g_pSolverConjGrad))) return 1;
	printf("done\n");

	printf("Creating buffers and filling them with initial data...");
	// The number of elements in a buffer to be tested
	for (auto i = 0u; i < vSize.z; ++i) {
		for (auto j = 0u; j < vSize.y; ++j) {
			for (auto k = 0u; k < vSize.x; ++k) {
				b[i * vSize.x * vSize.y + j * vSize.x + k] = (rand() % 256 - 255) / 255.0f;
			}
		}
	}

	CreateTexture3D(g_pDevice, DXGI_FORMAT_R32_FLOAT, vSize, b, &g_pb);
	CreateTexture3D(g_pDevice, DXGI_FORMAT_R32_FLOAT, vSize, nullptr, &g_px);
	CreateTexture3D(g_pDevice, DXGI_FORMAT_R32_FLOAT, vSize, nullptr, &g_px_CG);

#if defined(_DEBUG) || defined(PROFILE)
	if (g_pb)
		g_pb->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("b") - 1, "b");
	if (g_px)
		g_px->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("x") - 1, "x");
	if (g_px_CG)
		g_px_CG->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("x_CG") - 1, "x_CG");
#endif

	printf("done\n");

	printf("Creating buffer views...");
	CreateTexture3DSRV(g_pDevice, g_pb, &g_pbSRV);
	CreateTexture3DUAV(g_pDevice, g_px, &g_pxUAV);
	CreateTexture3DUAV(g_pDevice, g_px_CG, &g_pxUAV_CG);

#if defined(_DEBUG) || defined(PROFILE)
	if (g_pbSRV)
		g_pbSRV->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("b SRV") - 1, "b SRV");
	if (g_pxUAV)
		g_pxUAV->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("x UAV") - 1, "x UAV");
	if (g_pxUAV_CG)
		g_pxUAV_CG->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("x UAV CG") - 1, "x UAV CG");
#endif

	printf("done\n");

	ID3D11Query *pQueryDisjoint, *pQueryStart, *pQueryEnd;
	auto desc = CD3D11_QUERY_DESC(D3D11_QUERY_TIMESTAMP_DISJOINT);
	g_pDevice->CreateQuery(&desc, &pQueryDisjoint);
	desc.Query = D3D11_QUERY_TIMESTAMP;
	g_pDevice->CreateQuery(&desc, &pQueryStart);
	g_pDevice->CreateQuery(&desc, &pQueryEnd);

	printf("Solving by Jacobi iteration...");
	g_pSolverJacobi->Solve(vSize, g_pbSRV, g_pxUAV, 128);
	g_pContext->Begin(pQueryDisjoint);
	g_pContext->End(pQueryStart);
	for (auto i = 0u; i < ITERATIONS; ++i)
		g_pSolverJacobi->Solve(vSize, g_pbSRV, g_pxUAV, 128);
	g_pContext->End(pQueryEnd);
	g_pContext->End(pQueryDisjoint);
	
	UINT64 uStartTime, uEndTime;
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT freq;
	while (S_OK != g_pContext->GetData(pQueryStart, &uStartTime, sizeof(UINT64), 0));
	while (S_OK != g_pContext->GetData(pQueryEnd, &uEndTime, sizeof(UINT64), 0));
	while (S_OK != g_pContext->GetData(pQueryDisjoint, &freq, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0));
	double fTimeElapse = (uEndTime - uStartTime) / static_cast<double>(freq.Frequency) * 1000.0;
	printf("done (%.2fms)\n", fTimeElapse / double(ITERATIONS));
	
	printf("Solving by conjugate gradient...");
	g_pSolverConjGrad->Solve(vSize, g_pbSRV, g_pxUAV_CG, 17);
	g_pContext->Begin(pQueryDisjoint);
	g_pContext->End(pQueryStart);
	for (auto i = 0u; i < ITERATIONS; ++i)
		g_pSolverConjGrad->Solve(vSize, g_pbSRV, g_pxUAV_CG, 17);
	g_pContext->End(pQueryEnd);
	g_pContext->End(pQueryDisjoint);

	while (S_OK != g_pContext->GetData(pQueryStart, &uStartTime, sizeof(UINT64), 0));
	while (S_OK != g_pContext->GetData(pQueryEnd, &uEndTime, sizeof(UINT64), 0));
	while (S_OK != g_pContext->GetData(pQueryDisjoint, &freq, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0));
	fTimeElapse = (uEndTime - uStartTime) / static_cast<double>(freq.Frequency) * 1000.0;
	printf("done (%.2fms)\n", fTimeElapse / double(ITERATIONS));

	SAFE_RELEASE(pQueryEnd);
	SAFE_RELEASE(pQueryStart);
	SAFE_RELEASE(pQueryDisjoint);

	// Read back the result from GPU, verify its correctness against result computed by CPU
	{
		auto pRead = CreateAndCopyToDebugTex(g_pDevice, g_pContext, g_px);
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		float *p;
		g_pContext->Map(pRead, 0, D3D11_MAP_READ, 0, &MappedResource);

		// Set a break point here and put down the expression "p, 1024" in your watch window to see what has been written out by our CS
		// This is also a common trick to debug CS programs.
		p = (float*)MappedResource.pData;

		// Verify that if Compute Shader has done right
		printf("Print result (by Jacobi iteration)...\n");
		auto uPitch = max(vSize.x, 32);
		for (auto i = 0u; i < vSize.z; ++i) {
			for (auto j = 0u; j < vSize.y; ++j) {
				for (auto k = 0u; k < vSize.x; ++k) {
					printf("%.4f ", p[i * uPitch * vSize.y + j * uPitch + k]);
				}
				printf("\n");
			}
			printf("\n");
		}

		g_pContext->Unmap(pRead, 0);

		SAFE_RELEASE(pRead);
	}

	{
		ID3D11Texture3D* pRead = CreateAndCopyToDebugTex(g_pDevice, g_pContext, g_px_CG);
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		float *p;
		g_pContext->Map(pRead, 0, D3D11_MAP_READ, 0, &MappedResource);

		// Set a break point here and put down the expression "p, 1024" in your watch window to see what has been written out by our CS
		// This is also a common trick to debug CS programs.
		p = (float*)MappedResource.pData;

		// Verify that if Compute Shader has done right
		printf("Print result (by conjugate gradient)...\n");
		auto uPitch = max(vSize.x, 32);
		for (auto i = 0u; i < vSize.z; ++i) {
			for (auto j = 0u; j < vSize.y; ++j) {
				for (auto k = 0u; k < vSize.x; ++k) {
					printf("%.4f ", p[i * uPitch * vSize.y + j * uPitch + k]);
				}
				printf("\n");
			}
			printf("\n");
		}

		g_pContext->Unmap(pRead, 0);

		SAFE_RELEASE(pRead);
	}

	printf("Cleaning up...\n");
	SAFE_RELEASE(g_pxUAV_CG);
	SAFE_RELEASE(g_px_CG);
	SAFE_RELEASE(g_pSolverConjGrad);

	SAFE_RELEASE(g_pxUAV);
	SAFE_RELEASE(g_pbSRV);
	SAFE_RELEASE(g_pb);
	SAFE_RELEASE(g_px);
	SAFE_RELEASE(g_pSolverJacobi);

	SAFE_RELEASE(g_pContext);
	SAFE_RELEASE(g_pDevice);

	if (b) delete[] b;

	return 0;
}


//--------------------------------------------------------------------------------------
// Create the D3D device and device context suitable for running Compute Shaders(CS)
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateComputeDevice(ID3D11Device** ppDeviceOut, ID3D11DeviceContext** ppContextOut, bool bForceRef)
{
	*ppDeviceOut = nullptr;
	*ppContextOut = nullptr;

	HRESULT hr = S_OK;

	UINT uCreationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
	uCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL flOut;
	static const D3D_FEATURE_LEVEL flvl[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

	bool bNeedRefDevice = false;
	if (!bForceRef)
	{
		hr = D3D11CreateDevice(nullptr,                        // Use default graphics card
			D3D_DRIVER_TYPE_HARDWARE,    // Try to create a hardware accelerated device
			nullptr,                        // Do not use external software rasterizer module
			uCreationFlags,              // Device creation flags
			flvl,
			sizeof(flvl) / sizeof(D3D_FEATURE_LEVEL),
			D3D11_SDK_VERSION,           // SDK version
			ppDeviceOut,                 // Device out
			&flOut,                      // Actual feature level created
			ppContextOut);              // Context out

		if (SUCCEEDED(hr))
		{
			// A hardware accelerated device has been created, so check for Compute Shader support

			// If we have a device >= D3D_FEATURE_LEVEL_11_0 created, full CS5.0 support is guaranteed, no need for further checks
			if (flOut < D3D_FEATURE_LEVEL_11_0)
			{
#ifdef TEST_DOUBLE
				bNeedRefDevice = true;
				printf("No hardware Compute Shader 5.0 capable device found (required for doubles), trying to create ref device.\n");
#else
				// Otherwise, we need further check whether this device support CS4.x (Compute on 10)
				D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
				(*ppDeviceOut)->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
				if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
				{
					bNeedRefDevice = true;
					printf("No hardware Compute Shader capable device found, trying to create ref device.\n");
				}
#endif
			}

#ifdef TEST_DOUBLE
			else
			{
				// Double-precision support is an optional feature of CS 5.0
				D3D11_FEATURE_DATA_DOUBLES hwopts;
				(*ppDeviceOut)->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &hwopts, sizeof(hwopts));
				if (!hwopts.DoublePrecisionFloatShaderOps)
				{
					bNeedRefDevice = true;
					printf("No hardware double-precision capable device found, trying to create ref device.\n");
				}
			}
#endif
		}
	}

	if (bForceRef || FAILED(hr) || bNeedRefDevice)
	{
		// Either because of failure on creating a hardware device or hardware lacking CS capability, we create a ref device here

		SAFE_RELEASE(*ppDeviceOut);
		SAFE_RELEASE(*ppContextOut);

		hr = D3D11CreateDevice(nullptr,                        // Use default graphics card
			D3D_DRIVER_TYPE_REFERENCE,   // Try to create a hardware accelerated device
			nullptr,                        // Do not use external software rasterizer module
			uCreationFlags,              // Device creation flags
			flvl,
			sizeof(flvl) / sizeof(D3D_FEATURE_LEVEL),
			D3D11_SDK_VERSION,           // SDK version
			ppDeviceOut,                 // Device out
			&flOut,                      // Actual feature level created
			ppContextOut);              // Context out
		if (FAILED(hr))
		{
			printf("Reference rasterizer device create failure\n");
			return hr;
		}
	}

	return hr;
}

//--------------------------------------------------------------------------------------
// Compile and create the CS
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateComputeShader(LPCWSTR pSrcFile, LPCSTR pFunctionName,
	ID3D11Device* pDevice, ID3D11ComputeShader** ppShaderOut)
{
	if (!pDevice || !ppShaderOut)
		return E_INVALIDARG;

	// Finds the correct path for the shader file.
	// This is only required for this sample to be run correctly from within the Sample Browser,
	// in your own projects, these lines could be removed safely
	WCHAR str[MAX_PATH];
	HRESULT hr = FindDXSDKShaderFileCch(str, MAX_PATH, pSrcFile);
	if (FAILED(hr))
		return hr;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	const D3D_SHADER_MACRO defines[] =
	{
#ifdef USE_STRUCTURED_BUFFERS
		"USE_STRUCTURED_BUFFERS", "1",
#endif

#ifdef TEST_DOUBLE
		"TEST_DOUBLE", "1",
#endif
		nullptr, nullptr
	};

	// We generally prefer to use the higher CS shader profile when possible as CS 5.0 is better performance on 11-class hardware
	LPCSTR pProfile = (pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

	ID3DBlob* pErrorBlob = nullptr;
	ID3DBlob* pBlob = nullptr;

#if D3D_COMPILER_VERSION >= 46

	hr = D3DCompileFromFile(str, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, pFunctionName, pProfile,
		dwShaderFlags, 0, &pBlob, &pErrorBlob);

#else

	hr = D3DX11CompileFromFile(str, defines, nullptr, pFunctionName, pProfile,
		dwShaderFlags, 0, nullptr, &pBlob, &pErrorBlob, nullptr);

#endif

	if (FAILED(hr))
	{
		if (pErrorBlob)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

		SAFE_RELEASE(pErrorBlob);
		SAFE_RELEASE(pBlob);

		return hr;
	}

	hr = pDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppShaderOut);

	SAFE_RELEASE(pErrorBlob);
	SAFE_RELEASE(pBlob);

#if defined(_DEBUG) || defined(PROFILE)
	if (SUCCEEDED(hr))
	{
		(*ppShaderOut)->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(pFunctionName), pFunctionName);
	}
#endif

	return hr;
}

//--------------------------------------------------------------------------------------
// Create a CPU accessible buffer and download the content of a GPU buffer into it
// This function is very useful for debugging CS programs
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
ID3D11Buffer* CreateAndCopyToDebugBuf(ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer)
{
	ID3D11Buffer* debugbuf = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	pBuffer->GetDesc(&desc);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	if (SUCCEEDED(pDevice->CreateBuffer(&desc, nullptr, &debugbuf)))
	{
#if defined(_DEBUG) || defined(PROFILE)
		debugbuf->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Debug") - 1, "Debug");
#endif

		pd3dImmediateContext->CopyResource(debugbuf, pBuffer);
	}

	return debugbuf;
}

//--------------------------------------------------------------------------------------
// Create a CPU accessible texture and download the content of a GPU buffer into it
// This function is very useful for debugging CS programs
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
ID3D11Texture3D* CreateAndCopyToDebugTex(ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Texture3D* pTex)
{
	ID3D11Texture3D* debugTex = nullptr;

	D3D11_TEXTURE3D_DESC desc;
	pTex->GetDesc(&desc);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	if (SUCCEEDED(pDevice->CreateTexture3D(&desc, nullptr, &debugTex)))
	{
#if defined(_DEBUG) || defined(PROFILE)
		debugbuf->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Debug") - 1, "Debug");
#endif

		pd3dImmediateContext->CopyResource(debugTex, pTex);
	}

	return debugTex;
}

//--------------------------------------------------------------------------------------
// Tries to find the location of the shader file
// This is a trimmed down version of DXUTFindDXSDKMediaFileCch.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT FindDXSDKShaderFileCch(WCHAR* strDestPath,
	int cchDest,
	LPCWSTR strFilename)
{
	if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10)
		return E_INVALIDARG;

	// Get the exe name, and exe path
	WCHAR strExePath[MAX_PATH] =
	{
		0
	};
	WCHAR strExeName[MAX_PATH] =
	{
		0
	};
	WCHAR* strLastSlash = nullptr;
	GetModuleFileName(nullptr, strExePath, MAX_PATH);
	strExePath[MAX_PATH - 1] = 0;
	strLastSlash = wcsrchr(strExePath, TEXT('\\'));
	if (strLastSlash)
	{
		wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);

		// Chop the exe name from the exe path
		*strLastSlash = 0;

		// Chop the .exe from the exe name
		strLastSlash = wcsrchr(strExeName, TEXT('.'));
		if (strLastSlash)
			*strLastSlash = 0;
	}

	// Search in directories:
	//      .\
	//      %EXE_DIR%\..\..\%EXE_NAME%

	wcscpy_s(strDestPath, cchDest, strFilename);
	if (GetFileAttributes(strDestPath) != 0xFFFFFFFF)
		return S_OK;

	swprintf_s(strDestPath, cchDest, L"%s\\..\\..\\%s\\%s", strExePath, strExeName, strFilename);
	if (GetFileAttributes(strDestPath) != 0xFFFFFFFF)
		return S_OK;

	// On failure, return the file as the path but also return an error code
	wcscpy_s(strDestPath, cchDest, strFilename);

	return E_FAIL;
}
