// [[vk::push_constant]] PCS pcs;
Texture3D<float4> lightField : register(t0);
RWTexture2D<float> disparityTex : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 threadIdx : SV_DispatchThreadID)
{
    float4 color = lightField[uint3(threadIdx.x, threadIdx.y, 0)];
    float greyscale = color.x / 3.0f + color.y / 3.0f + color.z / 3.0f;
    disparityTex[threadIdx.xy] = greyscale;
}