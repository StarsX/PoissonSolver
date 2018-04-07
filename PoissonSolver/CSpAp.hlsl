#include "CSAx.hlsli"
#include "CHConjGrad.hlsli"

RWBuffer<float>		Ap;
RWBuffer<float>		pAp;

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, THREAD_GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float p;
	const float Ax = computeAx(DTid, p);
	
	const uint i = GETIDX(DTid);
	Ap[i] = Ax;
	pAp[i] = p * Ax;
}
