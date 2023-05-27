Texture3D<float4> lightField : register(t0);
RWTexture2D<float4> disparityTex : register(u1);

// push constant for runtime control
struct PCS { uint iPhase; uint nSteps; };
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
    for (int x = 0; x < PATCH_NX; x++) {
        for (int y = 0; y < PATCH_NY; y++) {
            for (int u = 0; u < CAMERA_NU; u++) {
                for (int v = 0; v < CAMERA_NV; v++) {
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

void phase_0(int3 threadIdx) {
    // calc and write disparities to texture
    float4 gradients = get_gradients(threadIdx);
    float2 disparity = get_disparity(gradients);
    disparityTex[threadIdx.xy].xy = disparity;
}
void phase_1(int3 threadIdx) {
    // sobel operator on 3x3 patch
    float3x3 pixelPatch;
    for (int x = 0; x < PATCH_NX; x++) {
        for (int y = 0; y < PATCH_NY; y++) {
            pixelPatch[x][y] = disparityTex[threadIdx.xy + int2(x - 1, y - 1)].x;
        }
    }

    float3x3 hori = {
        1, 0, -1,
        2, 0, -2,
        1, 0, -1
    };
    float3x3 veri = {
        1, 2, 1,
        0, 0, 0,
        -1, -2, -1
    };

    // component-wise multiplication
    hori = pixelPatch * hori;
    veri = pixelPatch * veri;

    // square component-wise (theres no dot for float3x3, only for float3)
    float accHori = 0.0f, accVeri = 0.0f;
    for (int i = 0; i < 3; i++) {
        accHori += dot(hori[i], hori[i]);
        accVeri += dot(veri[i], veri[i]);
    }
    float2 disparity;
    disparity.x = sqrt(accHori + accVeri) * 0.5f;


    float4 output;
    output.xy = disparity;
    output.zw = 0.0f;

    float cutoff = 0.00005f * (float)pcs.nSteps;
    if (disparity.y < cutoff) output.z = 1.0f; // show black dot for "uncertain"
    disparityTex[threadIdx.xy] = output;
}

[numthreads(GROUP_NX, GROUP_NY, 1)]
void main(int3 threadIdx : SV_DispatchThreadID, int3 localIdx : SV_GroupThreadID)
{
    switch (pcs.iPhase) {
        case 0: phase_0(threadIdx); break;
        case 1: phase_1(threadIdx); break;
    }
}