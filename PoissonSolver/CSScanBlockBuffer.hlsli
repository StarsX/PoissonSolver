
#define NUM_THREADS_PER_GROUP		1024
#define NUM_THREADS_PER_WAVEFRONT	(1 << NUM_BITSHIFTS)
#define NUM_BITSHIFTS				5
#define NUM_WAVEFRONTS				(NUM_THREADS_PER_GROUP / NUM_THREADS_PER_WAVEFRONT)

//--------------------------------------------------------------------------------------
// Unordered Access Buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float>			g_RWPreSumInc	: register(u1);

//--------------------------------------------------------------------------------------
// Groupshared memory.
//--------------------------------------------------------------------------------------
groupshared float g_BlockBuffer[NUM_THREADS_PER_GROUP];

//--------------------------------------------------------------------------------------
// Does an inclusive prefix sum on g_BlockBuffer.
//--------------------------------------------------------------------------------------
inline void ScanBlockBuffer(uint uIdx, uint uWTid, uint uWid)
{
	// Step 1, prefix sum for each wavefront
	for (uint uStride = 1; uStride < NUM_THREADS_PER_WAVEFRONT; uStride <<= 1)
		g_BlockBuffer[uIdx] += uWTid >= uStride ? g_BlockBuffer[uIdx - uStride] : 0.0;

	// Step 2 + 3, cumulative result of the previous wave
	GroupMemoryBarrierWithGroupSync();
	float fGroupScanResult = 0.0;
	[unroll]
	for (uint i = 1; i < NUM_WAVEFRONTS; ++i)
		fGroupScanResult += uWid >= i ? g_BlockBuffer[i * NUM_THREADS_PER_WAVEFRONT - 1] : 0.0;

	// Step 4, finish prefix sum of each group
	GroupMemoryBarrierWithGroupSync();
	g_BlockBuffer[uIdx] += fGroupScanResult;
}
