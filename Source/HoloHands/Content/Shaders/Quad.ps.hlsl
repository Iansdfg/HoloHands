struct PixelShaderInput
{
    min16float4 pos : SV_POSITION;
    min16float2 texCoord : TEXCOORD1;
};

Texture2D quadTexture : register(t0);
SamplerState quadSampler : register(s0);

min16float4 main(PixelShaderInput input) : SV_TARGET
{
    min16float depth = quadTexture.Sample(quadSampler, input.texCoord).r * 10.0;
    return min16float4(depth, depth, depth, 1.0f);
}
