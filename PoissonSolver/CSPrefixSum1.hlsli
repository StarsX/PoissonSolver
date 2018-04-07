//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "CSScanBlockBuffer.hlsli"

//--------------------------------------------------------------------------------------
// Unordered Access Buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<typeless>	g_RWPerBinValues	: register(u0);

//--------------------------------------------------------------------------------------
// Prefix sum on g_RWPerBinCount and saving the highest results in g_RWPreSumInc.
//--------------------------------------------------------------------------------------
[numthreads(NUM_THREADS_PER_GROUP, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	g_BlockBuffer[GTid.x] = g_RWPerBinValues[DTid.x];

	const uint uWid = GTid.x >> NUM_BITSHIFTS;
	const uint uWTid = GTid.x - (uWid << NUM_BITSHIFTS);

	GroupMemoryBarrierWithGroupSync();
	ScanBlockBuffer(GTid.x, uWTid, uWid);					// Scan for each group

	GroupMemoryBarrierWithGroupSync();
	g_RWPerBinValues[DTid.x + 1] = g_BlockBuffer[GTid.x];	// Write back group local prefix sums
	if (GTid.x == NUM_THREADS_PER_GROUP - 1)
		g_RWPreSumInc[Gid.x] = g_BlockBuffer[GTid.x];		// Save highest value of each group
}
