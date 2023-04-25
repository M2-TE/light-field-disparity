float4 main(uint vertID : SV_VertexID) : SV_Position
{
    return float4((vertID == 0) ? 3.0f : -1.0f, (vertID == 2) ? -3.0f : 1.0f, 1.0f, 1.0f);
}