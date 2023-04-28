Texture3D<float4> lightField : register(t0);
RWTexture2D<float> disparityTex : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 threadIdx : SV_DispatchThreadID) 
{
    disparityTex[threadIdx.xy] = lightField[uint3(threadIdx.x, threadIdx.y, 0)].r;
}