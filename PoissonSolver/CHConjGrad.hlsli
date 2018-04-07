//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

#define uSlice		(m_vDim.x * m_vDim.y)
#define GETIDX(v)	(v.z * uSlice + v.y * m_vDim.x + v.x)

cbuffer cbDimension
{
	uint4 m_vDim;
};
