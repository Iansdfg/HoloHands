struct PixelShaderInput
{
    min16float4 pos : SV_POSITION;
    min16float2 texCoord : TEXCOORD1;
};

Texture2D quadTexture : register(t0);
SamplerState quadSampler : register(s0);

min16float4 main(PixelShaderInput input) : SV_TARGET
{
   const float min = 0.00;
   const float max = 0.020;

   float depth = quadTexture.Sample(quadSampler, input.texCoord).r;
   /*
   float color = (depth - min) / (max - min); //normalize to min, max rang.e
   color = 1 - color; //invert.

   if (color <= 0 || color >= 1.0)
   {
      return min16float4(1, 0, 0, 0); //out of range.
   }
   */

    //return min16float4(color, color, color, 1);
    return min16float4(depth, depth, depth, 1);
}
