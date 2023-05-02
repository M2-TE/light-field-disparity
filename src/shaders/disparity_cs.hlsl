// [[vk::push_constant]] PCS pcs;
Texture3D<float4> lightField : register(t0);
RWTexture2D<float> disparityTex : register(u1);

#define BRIGHTNESS_STANDARD(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
#define BRIGHTNESS_REAL(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction

[numthreads(1, 1, 1)]
void main(uint3 threadIdx : SV_DispatchThreadID)
{
    float4 color = lightField[uint3(threadIdx.x, threadIdx.y, 8)];
    float greyscale = BRIGHTNESS_STANDARD(color.rgb);
    disparityTex[threadIdx.xy] = greyscale;
}