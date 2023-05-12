Texture2D<float2> disparityTex : register(t0);

float4 get_heat(float val)
{
    // heatmap view (https://www.shadertoy.com/view/WslGRN)
    float heatLvl = val * 3.14159265 / 2;
    return float4(sin(heatLvl), sin(heatLvl * 2), cos(heatLvl), 1.0f);
}

float4 main(float4 inputPos : SV_Position) : SV_Target
{
    // confidence cutoff
    float2 disparity = disparityTex[(uint2)inputPos.xy];
    float4 heatCol = get_heat(disparity.x);
    if (disparity.y < 0.00005) heatCol = 0.0f;
    return heatCol;
}
