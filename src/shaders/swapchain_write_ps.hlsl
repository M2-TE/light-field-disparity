[[vk::input_attachment_index(0)]] SubpassInput<float> input;
//Texture2D image : register(t0);

float main(float4 inputPos : SV_Position) : SV_Target
{
    //uint3 pos = uint3(inputPos.x, inputPos.y, 1);
    //return lightfieldImages[pos];

    float col = input.SubpassLoad();
    col.r = 1.0f;
    
    return col;
}