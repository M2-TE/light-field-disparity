Texture3D<float4> lightField : register(t0);
RWTexture2D<float2> disparityTex : register(u1);

// push constant for runtime control
struct PCS { uint nSteps; };
[[vk::push_constant]] PCS pcs;

#define BRIGHTNESS_GREY(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
#define BRIGHTNESS_REAL(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction
// compute group patch size
#define GROUP_NX 16
#define GROUP_NY 16
// camera patch size
#define CAMERA_NU 3
#define CAMERA_NV 3
// pixel patch size
#define PATCH_NX 3
#define PATCH_NY 3

float4 get_gradients(int3 threadIdx) {
    // cam-specific filters
    float p[] = { 0.229879f, 0.540242f, 0.229879f };
    float d[] = { -0.425287f, 0.000000f, 0.425287f };
    
    // lightfield derivatives
    float Lx = 0.0f, Ly = 0.0f;
    float Lu = 0.0f, Lv = 0.0f;

    // iterate over 2D patch of pixels (3x3)
    for (int x = 0; x < PATCH_NX; x++)
    {
        for (int y = 0; y < PATCH_NY; y++)
        {
            for (int u = 0; u < CAMERA_NU; u++)
            {
                for (int v = 0; v < CAMERA_NV; v++)
                {
                    int camIndex = u * 3 + v;
                    int3 texOffset = int3(x - 1, y - 1, camIndex);

                    float3 color = lightField[threadIdx + texOffset].rgb;
                    float luma = BRIGHTNESS_GREY(color);
                    
                    // approximate derivatives using 3-tap filter
                    Lx += d[x] * p[y] * p[u] * p[v] * luma;
                    Ly += p[x] * d[y] * p[u] * p[v] * luma;
                    Lu += p[x] * p[y] * d[u] * p[v] * luma;
                    Lv += p[x] * p[y] * p[u] * d[v] * luma;
                }
            }
        }
    }
    
    return float4(Lx, Ly, Lu, Lv);
}
float2 get_disparity(float4 gradients) {
    float a = gradients.x * gradients.z + gradients.y * gradients.w;
    float confidence = gradients.x * gradients.x + gradients.y * gradients.y;
    float disparity = a / confidence;
    return float2(disparity, confidence);
}

[numthreads(GROUP_NX, GROUP_NY, 1)]
void main(int3 threadIdx : SV_DispatchThreadID, int3 localIdx : SV_GroupThreadID)
{
    // calc and write disparities to shared mem
    float4 gradients = get_gradients(threadIdx);
    float2 disparity = get_disparity(gradients);
    disparityTex[localIdx.xy] = disparity;
    DeviceMemoryBarrierWithGroupSync();

    // calculate disparity from gradients

    // do some wizzy shit
    float cutoff = 0.00005f;
    float2 accumulatedDisparity = 0.0f;
    int totalAccumulations = 0;
    // look for confident disparity in neighbouring pixels
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            // accumulate disparity if it meets threshhold
            float2 tmpDisparity = disparityTex[threadIdx.xy + int2(x, y)];
            accumulatedDisparity += tmpDisparity.y > cutoff ? tmpDisparity : 0.0f;
            totalAccumulations += tmpDisparity.y > cutoff ? 1 : 0;
        }
    }

    // average out valid neighbours
    accumulatedDisparity = accumulatedDisparity / (float)totalAccumulations;
    if (pcs.nSteps == 1) {
        disparity = disparity.y > cutoff ? disparity : accumulatedDisparity;
    }
    
    disparityTex[threadIdx.xy] = disparity;
}