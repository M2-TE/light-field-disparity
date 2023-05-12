Texture2D<uint4> disparityTex : register(t0);

float4 get_heat(float val)
{
    // heatmap view (https://www.shadertoy.com/view/WslGRN)
    float heatLvl = val * 3.14159265 / 2;
    return float4(sin(heatLvl), sin(heatLvl * 2), cos(heatLvl), 1.0f);
}

float4 main(float4 inputPos : SV_Position) : SV_Target
{
    uint2 texPos = uint2(inputPos.x, inputPos.y);
    uint4 input = disparityTex[texPos];
    double2 disparity = asdouble(input.xz, input.yw);

    // confidence cutoff
    float4 heatCol = get_heat((float)disparity.x);
    //float4 heatCol = (float)disparity.xxxx;
    if (disparity.y < 0.00005) heatCol = 0.0f;
    return heatCol;
}
