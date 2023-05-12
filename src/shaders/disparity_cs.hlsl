// [[vk::push_constant]] PCS pcs;
Texture3D<float4> lightField : register(t0);
RWTexture2D<uint4> disparityTex : register(u1);

#define BRIGHTNESS_GREY(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
#define BRIGHTNESS_REAL(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction
// compute group patch size
#define GROUP_NX 16
#define GROUP_NY 16
// camera patch size
#define CAMERA_NX 3
#define CAMERA_NY 3

float4 get_gradients(uint3 localIdx) {
    // cam-specific filters
    float p[] = { 0.229879f, 0.540242f, 0.229879f };
    float d[] = { -0.425287f, 0.000000f, 0.425287f };
    
    // lightfield derivatives
    float Lx = 0.0f, Ly = 0.0f;
    float Lu = 0.0f, Lv = 0.0f;

    int nCams = 3; // cams in one dimension
    int nPixels = 3; // pixels in one dimension
    int pixelOffset = nPixels / 2;
    
    // iterate over 2D patch of pixels (3x3)
    for (int x = 0; x < nPixels; x++)
    {
        for (int y = 0; y < nPixels; y++)
        {
            for (int u = 0; u < nCams; u++)
            {
                for (int v = 0; v < nCams; v++)
                {
                    int camIndex = u * 3 + v;
                    int3 texOffset = int3(x - pixelOffset, y - pixelOffset, camIndex);

                    float3 color = lightField[uint3(texPos + texOffset)].rgb;
                    
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
double2 get_disparity(double4 gradients) {
    // get disparity and confidence
    double a = gradients.x * gradients.z + gradients.y * gradients.w;
    double confidence = gradients.x * gradients.x + gradients.y * gradients.y;
    double disparity = a / confidence;
    return double2(disparity, confidence);
}

groupshared float pLightField[GROUP_NX][GROUP_NY][CAMERA_NX][CAMERA_NY]; // 4D light field (3x3 pixel and camera patches)

[numthreads(GROUP_NX, GROUP_NY, 1)] // 16x16 pixel group
void main(uint3 threadIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID)
{  
    int3 texPos = int3(threadIdx.x, threadIdx.y, 0);

    for (int x = 0; x < CAMERA_NX; x++) {
        for (int y = 0; y < CAMERA_NY; y++) {

            int3 offset = int3(0, 0, x + y * CAMERA_NX);
            float color = BRIGHTNESS_GREY(lightField[texPos + offset].rgb);
            pLightField[localIdx.x][localIdx.y][x][y] = color;
        }
    }

    float4 gradients = get_gradients(localIdx);
    double2 disparity = get_disparity((double4)gradients);


    AllMemoryBarrierWithGroupSync();

    // TODO: share this data using shared memory instead
    float2 pixelPatch[3][3];
    for (x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            pixelPatch[x][y] = disparityTex[texPos.xy + int2(x, y)].xy;
        }
    }

    // write output in double precision
    uint4 output;
    asuint(disparity.x, output.x, output.y);
    asuint(disparity.y, output.z, output.w);
    disparityTex[texPos.xy] = output;
}