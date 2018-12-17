struct PixelShaderInput
{
    min16float4 pos : SV_POSITION;
    min16float2 texCoord : TEXCOORD1;
};

Texture2D quadTexture : register(t0);
SamplerState quadSampler : register(s0);

min16float4 main(PixelShaderInput input) : SV_TARGET
{
    min16float3 texel = (min16float3)(quadTexture.Sample(quadSampler, input.texCoord));
    return min16float4(texel, 1.0f);
}
