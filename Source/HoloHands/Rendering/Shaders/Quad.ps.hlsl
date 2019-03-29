struct PixelShaderInput
{
   min16float4 position : SV_POSITION;
   min16float2 textureCoords : TEXCOORD1;
};

Texture2D quadTexture : register(t0);
SamplerState quadSampler : register(s0);

min16float4 main(PixelShaderInput input) : SV_TARGET
{
   min16float4 color;

   const min16float2 min = min16float2(0.01, 0.01);
   const min16float2 max = min16float2(0.99, 0.99);
   if (input.textureCoords.x > max.x ||
      input.textureCoords.y > max.y ||
      input.textureCoords.x < min.x ||
      input.textureCoords.y < min.y)
   {
      //Draw border.
      return min16float4(1, 0, 0, 1);
   }
   else
   {
      //Draw texture.
      float texel = quadTexture.Sample(quadSampler, input.textureCoords).r;
      return min16float4(texel, texel, texel, 1);

   }
}
