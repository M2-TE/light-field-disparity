// [[vk::push_constant]] PCS pcs;
Texture3D<float4> lightField : register(t0);
RWTexture2D<float4> disparityTex : register(u1);

#define BRIGHTNESS_GREY(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
#define BRIGHTNESS_REAL(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction

float4 get_gradients(int3 texPos) {
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
    // get disparity and confidence
    float a = gradients.x * gradients.z + gradients.y * gradients.w;
    float confidence = gradients.x * gradients.x + gradients.y * gradients.y;
    float disparity = a / confidence;
    return float2(disparity, confidence);
}

[numthreads(1, 1, 1)]
void main(uint3 threadIdx : SV_DispatchThreadID)
{  
    uint3 texPos = uint3(threadIdx.x, threadIdx.y, 0);

    float4 gradients = get_gradients(texPos);
    float2 disparity = get_disparity(gradients);

    disparityTex[texPos.xy].xy = disparity.xy;
}