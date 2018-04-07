//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

Texture3D<float>			x;

float computeAx(int3 vTex, inout float q)
{
	q = x[vTex];

	float Ax = -6.0 * q;
	Ax += x[vTex + int3(-1, 0, 0)];
	Ax += x[vTex + int3(1, 0, 0)];
	Ax += x[vTex + int3(0, -1, 0)];
	Ax += x[vTex + int3(0, 1, 0)];
	Ax += x[vTex + int3(0, 0, -1)];
	Ax += x[vTex + int3(0, 0, 1)];

	return Ax;
}
