#pragma once

namespace HoloHands
{
    struct ModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
    };

    // Assert that the constant buffer remains 16-byte aligned (best practice).
    static_assert((sizeof(ModelConstantBuffer) % (sizeof(float) * 4)) == 0, "Model constant buffer size must be 16-byte aligned (16 bytes is the length of four floats).");

    struct VertexPositionTextureCoords
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 texCoord;
    };
}