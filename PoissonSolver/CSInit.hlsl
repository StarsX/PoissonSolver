//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#define TEXTURE3D	RWTexture3D
#include "CSAx.hlsli"
#include "CHConjGrad.hlsli"

Texture3D<float>			b;

RWTexture3D<float>			r;
RWStructuredBuffer<float>	rr;
RWTexture3D<float>			p;

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, THREAD_GROUP_SIZE)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float x0;
	const float Ax = computeAx(DTid, x0);

	uint3 vDim;
	b.GetDimensions(vDim.x, vDim.y, vDim.z);

	const float r0 = b[DTid] - Ax;
	r[DTid] = r0;
	p[DTid] = r0;
	rr[GETIDX(DTid)] = r0 * r0;
}
