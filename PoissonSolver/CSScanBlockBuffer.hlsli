//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#define NUM_THREADS_PER_GROUP	1024
#define NUM_THREADS_PER_WAVE	(1 << NUM_BITSHIFTS)
#define NUM_BITSHIFTS			5
#define NUM_WAVES				(NUM_THREADS_PER_GROUP / NUM_THREADS_PER_WAVE)

//--------------------------------------------------------------------------------------
// Unordered Access Buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<typeless>	g_RWDst;
RWStructuredBuffer<typeless>	g_RWInc;
#ifndef g_RWSrc
RWStructuredBuffer<typeless>	g_RWSrc;
#endif

//--------------------------------------------------------------------------------------
// Groupshared memory.
//--------------------------------------------------------------------------------------
groupshared typeless g_BlockBuffer[NUM_THREADS_PER_GROUP];

//--------------------------------------------------------------------------------------
// Does an inclusive prefix sum on g_BlockBuffer.
//--------------------------------------------------------------------------------------
inline void ScanBlockBuffer(uint uIdx, uint uWTid, uint uWid)
{
	// Step 1, prefix sum for each wavefront
	for (uint uStride = 1; uStride < NUM_THREADS_PER_WAVE; uStride <<= 1)
		g_BlockBuffer[uIdx] += uWTid >= uStride ? g_BlockBuffer[uIdx - uStride] : 0;

	// Step 2 + 3, cumulative result of the previous wave
	GroupMemoryBarrierWithGroupSync();
	typeless fGroupScanResult = 0;
	[unroll]
	for (uint i = 1; i < NUM_WAVES; ++i)
		fGroupScanResult += uWid >= i ? g_BlockBuffer[i * NUM_THREADS_PER_WAVE - 1] : 0;

	// Step 4, finish prefix sum of each group
	GroupMemoryBarrierWithGroupSync();
	g_BlockBuffer[uIdx] += fGroupScanResult;
}
