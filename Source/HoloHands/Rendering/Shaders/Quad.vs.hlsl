// A constant buffer that stores the model transform.
cbuffer ModelConstantBuffer : register(b0)
{
    float4x4 model;
};

// A constant buffer that stores each set of view and projection matrices in column-major format.
cbuffer ViewProjectionConstantBuffer : register(b1)
{
    float4x4 viewProjection[2];
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
    min16float3 pos : POSITION;
    min16float2 texCoord : TEXCOORD1;
    uint instId : SV_InstanceID;
};

// Per-vertex data passed to the geometry shader.
struct VertexShaderOutput
{
    min16float4 pos : SV_POSITION;
    min16float2 texCoord : TEXCOORD1;
    uint rtvId : SV_RenderTargetArrayIndex; // SV_InstanceID % 2
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    float4 pos = float4(input.pos, 1.0f);

    int idx = input.instId % 2;

    pos = mul(pos, model);

    pos = mul(pos, viewProjection[idx]);
    output.pos = (min16float4)pos;

    output.rtvId = idx;

    output.texCoord = input.texCoord;

    return output;
}
