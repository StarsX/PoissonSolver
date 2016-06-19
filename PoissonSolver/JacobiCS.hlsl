
Texture3D<float>	b;
RWTexture3D<float>	x;

//--------------------------------------------------------------------------------------
// Jacobi iteration
//--------------------------------------------------------------------------------------
float jacobi(half2 vf, int3 vTex)
{
	float q = vf.x * b[vTex];
	q += x[vTex + int3(-1, 0, 0)];
	q += x[vTex + int3(1, 0, 0)];
	q += x[vTex + int3(0, -1, 0)];
	q += x[vTex + int3(0, 1, 0)];
	q += x[vTex + int3(0, 0, -1)];
	q += x[vTex + int3(0, 0, 1)];

	return q / vf.y;
}

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const half2 vf = { -1.0, 6.0 };
	x[DTid] = jacobi(vf, DTid);
}
