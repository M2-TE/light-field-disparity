[[vk::input_attachment_index(0)]] SubpassInput input;
//Texture2D image : register(t0);

float4 main(float4 inputPos : SV_Position) : SV_Target
{
    //uint3 pos = uint3(inputPos.x, inputPos.y, 1);
    //return lightfieldImages[pos];

    float4 col = input.SubpassLoad();
    return col;
}