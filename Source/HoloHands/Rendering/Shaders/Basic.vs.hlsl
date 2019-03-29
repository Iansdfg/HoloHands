
cbuffer ModelConstantBuffer : register(b0)
{
    float4x4 model;
    float4 color;
};

cbuffer ViewProjectionConstantBuffer : register(b1)
{
    float4x4 viewProjection[2];
};

struct VertexShaderInput
{
    min16float3 position : POSITION;
    uint instanceId : SV_InstanceID;
};

struct VertexShaderOutput
{
    min16float4 position : SV_POSITION;
    min16float4 color : TEXCOORD0;
    uint rtvId : SV_RenderTargetArrayIndex;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    float4 position = float4(input.position, 1.0f);

    int index = input.instanceId % 2;

    //Apply transforms.
    position = mul(position, model);
    position = mul(position, viewProjection[index]);

    output.position = (min16float4)position;
    output.rtvId = index;
    output.color = (min16float4)color;

    return output;
}
