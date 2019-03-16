struct PixelShaderInput
{
   min16float4 pos : SV_POSITION;
   min16float2 texCoord : TEXCOORD1;
};

Texture2D quadTexture : register(t0);
SamplerState quadSampler : register(s0);

min16float4 main(PixelShaderInput input) : SV_TARGET
{
   min16float4 color;

   const min16float2 min = min16float2(0.01, 0.01);
   const min16float2 max = min16float2(0.99, 0.99);
   if (input.texCoord.x > max.x ||
      input.texCoord.y > max.y ||
      input.texCoord.x < min.x ||
      input.texCoord.y < min.y)
   {
      //Draw border.
      return min16float4(1, 0, 0, 1);
   }
   else
   {
      //Draw texture.
      float texel = quadTexture.Sample(quadSampler, input.texCoord).r;
      return min16float4(texel, texel, texel, 1);

   }
}
