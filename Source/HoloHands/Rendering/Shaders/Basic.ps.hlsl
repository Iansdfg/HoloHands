struct PixelShaderInput
{
   min16float4 position : SV_POSITION;
   min16float4 color : TEXCOORD0;
};

min16float4 main(PixelShaderInput input) : SV_TARGET
{
   return float4(input.color.rgb, 1);
}
