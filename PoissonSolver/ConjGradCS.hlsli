
#include "ConjGradCH.hlsli"

Texture3D<float>	p_RO;
Buffer<float>		Ap;
Buffer<float>		Acc_rr;
Buffer<float>		Acc_pAp;

RWTexture3D<float>	x;
RWBuffer<float>		r;
RWBuffer<float>		rr;

[numthreads(1, 1, 1)]
void update_x(uint3 DTid : SV_DispatchThreadID)
{
	const uint i = GETIDX(DTid);
	const uint iMax = m_vDim.x * m_vDim.y * m_vDim.z;
	const float a = Acc_rr[iMax] / Acc_pAp[iMax];

	x[DTid] += a * p_RO[DTid];
	
	const float ri = r[i] - a * Ap[i];
	r[i] = ri;
	rr[i] = ri * ri;
}


Buffer<float>		r_RO;
Buffer<float>		Acc_rr_new;

RWTexture3D<float>	p;

[numthreads(1, 1, 1)]
void update_p(uint3 DTid : SV_DispatchThreadID)
{
	const uint i = GETIDX(DTid);
	const uint iMax = m_vDim.x * m_vDim.y * m_vDim.z;
	const float b = Acc_rr_new[iMax] / Acc_rr[iMax];

	p[DTid] = r_RO[i] + b * p[DTid];
}
