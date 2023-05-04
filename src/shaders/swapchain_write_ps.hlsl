Texture2D<float> disparityTex : register(t0);

float4 get_heat(float val)
{
    // heatmap view (https://www.shadertoy.com/view/WslGRN)
    float heatLvl = val * 3.14159265 / 2;
    return float4(sin(heatLvl), sin(heatLvl * 2), cos(heatLvl), 1.0f);
}

float4 main(float4 inputPos : SV_Position) : SV_Target
{
    uint2 texPos = uint2(inputPos.x, inputPos.y);
    float disparity = disparityTex[texPos];
    // return get_heat(disparity);
    return disparity.rrrr;
}