#pragma once

namespace HoloHands
{
   struct ModelConstantBuffer
   {
      DirectX::XMFLOAT4X4 model;
   };

   struct CrosshairConstantBuffer
   {
      DirectX::XMFLOAT4X4 model;
      DirectX::XMFLOAT4 color;
   };

   struct VertexPositionTextureCoords
   {
      DirectX::XMFLOAT3 position;
      DirectX::XMFLOAT2 textureCoords;
   };

   struct VertexPosition
   {
      DirectX::XMFLOAT3 position;
   };
}