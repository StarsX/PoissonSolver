#include "CSScanBlockBuffer.hlsli"

//--------------------------------------------------------------------------------------
// Unordered Access Buffers
//--------------------------------------------------------------------------------------
RWBuffer<float>	g_RWPrefixSums	: register(u0);

//--------------------------------------------------------------------------------------
// Prefix sum on g_RWPreSumInc and writing the results back into g_RWPrefixSum.
//--------------------------------------------------------------------------------------
[numthreads(NUM_THREADS_PER_GROUP, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	g_BlockBuffer[GTid.x] = g_RWPreSumInc[GTid.x]; // This restricts the number of tiles to 1024 * 1024

	const uint uWid = GTid.x >> NUM_BITSHIFTS;
	const uint uWTid = GTid.x - (uWid << NUM_BITSHIFTS);

	GroupMemoryBarrierWithGroupSync();
	ScanBlockBuffer(GTid.x, uWTid, uWid);

	GroupMemoryBarrierWithGroupSync();
	g_RWPrefixSums[DTid.x + 1] += Gid.x > 0 ? g_BlockBuffer[Gid.x - 1] : 0;
}
