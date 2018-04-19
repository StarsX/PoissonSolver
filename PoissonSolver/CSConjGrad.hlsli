//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "CHConjGrad.hlsli"

//--------------------------------------------------------------------------------------
// Update x
//--------------------------------------------------------------------------------------
Texture3D<float>			p_RO;
Texture3D<float>			Ap;
StructuredBuffer<float>		Acc_rr;
StructuredBuffer<float>		Acc_pAp;

RWTexture3D<float>			x;
RWTexture3D<float>			r;
RWStructuredBuffer<float>	rr;

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, THREAD_GROUP_SIZE)]
void update_x(uint3 DTid : SV_DispatchThreadID)
{
	uint3 vDim;
	p_RO.GetDimensions(vDim.x, vDim.y, vDim.z);
	const uint iMax = vDim.x * vDim.y * vDim.z;
	const float a = Acc_rr[iMax] / Acc_pAp[iMax];

	x[DTid] += a * p_RO[DTid];
	
	const float ri = r[DTid] - a * Ap[DTid];
	r[DTid] = ri;
	rr[GETIDX(DTid)] = ri * ri;
}

//--------------------------------------------------------------------------------------
// Update p
//--------------------------------------------------------------------------------------
Texture3D<float>			r_RO;
StructuredBuffer<float>		Acc_rr_new;

RWTexture3D<float>			p;

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, THREAD_GROUP_SIZE)]
void update_p(uint3 DTid : SV_DispatchThreadID)
{
	uint3 vDim;
	r_RO.GetDimensions(vDim.x, vDim.y, vDim.z);
	const uint iMax = vDim.x * vDim.y * vDim.z;
	const float b = Acc_rr_new[iMax] / Acc_rr[iMax];

	p[DTid] = r_RO[DTid] + b * p[DTid];
}
