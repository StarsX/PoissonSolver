
#include "AxCS.hlsli"
#include "ConjGradCH.hlsli"

Texture3D<float>	b;

RWBuffer<float>		r;
RWBuffer<float>		rr;
RWTexture3D<float>	p;

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float x0;
	const float Ax = computeAx(DTid, x0);

	const uint i = GETIDX(DTid);
	const float r0 = b[DTid] - Ax;
	r[i] = r0;
	rr[i] = r0 * r0;
	p[DTid] = r0;
}
