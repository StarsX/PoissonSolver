
#include "AxCS.hlsli"
#include "ConjGradCH.hlsli"

RWBuffer<float>		Ap;
RWBuffer<float>		pAp;

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float p;
	const float Ax = computeAx(DTid, p);
	
	const uint i = GETIDX(DTid);
	Ap[i] = Ax;
	pAp[i] = p * Ax;
}
