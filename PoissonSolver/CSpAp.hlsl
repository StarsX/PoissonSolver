//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#define TEXTURE3D	Texture3D
#include "CSAx.hlsli"
#include "CHConjGrad.hlsli"

RWTexture3D<float>			Ap;
RWStructuredBuffer<float>	pAp;

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, THREAD_GROUP_SIZE)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float p;
	const float Ax = computeAx(DTid, p);

	uint3 vDim;
	x.GetDimensions(vDim.x, vDim.y, vDim.z);

	Ap[DTid] = Ax;
	pAp[GETIDX(DTid)] = p * Ax;
}
