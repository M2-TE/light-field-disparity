Texture2D<float> disparityTex : register(t0);

float4 main(float4 inputPos : SV_Position) : SV_Target
{
    uint2 pos = uint2(inputPos.x, inputPos.y);
    return disparityTex[pos].rrrr;
}