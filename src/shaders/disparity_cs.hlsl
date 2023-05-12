// [[vk::push_constant]] PCS pcs;
Texture3D<float4> lightField : register(t0);
RWTexture2D<uint4> disparityTex : register(u1);

#define BRIGHTNESS_GREY(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
#define BRIGHTNESS_REAL(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction
// compute group patch size
#define GROUP_NX 16
#define GROUP_NY 16
// camera patch size
#define CAMERA_NU 3
#define CAMERA_NV 3
// pixel patc size
#define PATCH_NX CAMERA_NU
#define PATCH_NY CAMERA_NV
groupshared double4 pGradients[GROUP_NX][GROUP_NY]; // 2D gradient map

double4 get_gradients(int3 threadIdx) {
    // cam-specific filters
    double p[] = { 0.229879f, 0.540242f, 0.229879f };
    double d[] = { -0.425287f, 0.000000f, 0.425287f };
    
    // lightfield derivatives
    double Lx = 0.0f, Ly = 0.0f;
    double Lu = 0.0f, Lv = 0.0f;

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
                    double luma = (double) BRIGHTNESS_GREY(color);
                    
                    // approximate derivatives using 3-tap filter
                    Lx += d[x] * p[y] * p[u] * p[v] * luma;
                    Ly += p[x] * d[y] * p[u] * p[v] * luma;
                    Lu += p[x] * p[y] * d[u] * p[v] * luma;
                    Lv += p[x] * p[y] * p[u] * d[v] * luma;
                }
            }
        }
    }
    
    return double4(Lx, Ly, Lu, Lv);
}
double2 get_disparity(double4 gradients) {
    // get disparity and confidence
    double a = gradients.x * gradients.z + gradients.y * gradients.w;
    double confidence = gradients.x * gradients.x + gradients.y * gradients.y;
    double disparity = a / confidence;
    return double2(disparity, confidence);
}

[numthreads(GROUP_NX, GROUP_NY, 1)]
void main(int3 threadIdx : SV_DispatchThreadID, int3 localIdx : SV_GroupThreadID)
{
    // write gradients to shared mem
    pGradients[localIdx.x][localIdx.y] = get_gradients(threadIdx);
    GroupMemoryBarrierWithGroupSync();


    // calculate disparity from gradients
    double2 disparity = get_disparity(pGradients[localIdx.x][localIdx.y]);


    // write output in double precision
    uint4 output;
    asuint(disparity.x, output.x, output.y);
    asuint(disparity.y, output.z, output.w);
    disparityTex[threadIdx.xy] = output;
}