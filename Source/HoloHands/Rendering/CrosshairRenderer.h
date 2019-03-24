#pragma once

#include "Rendering/Shaders/ShaderStructs.h"

namespace HoloHands
{
   class CrosshairRenderer
   {
   public:
      CrosshairRenderer(const std::shared_ptr<Graphics::DeviceResources>& deviceResources);
      void CreateDeviceDependentResources();
      void ReleaseDeviceDependentResources();

      void Update();
      void Render();

      void SetPosition(const Windows::Foundation::Numerics::float3& position);
      void SetColor(const Windows::Foundation::Numerics::float3& color);

   private:
      Graphics::DeviceResourcesPtr _deviceResources;

      Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
      Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;

      Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
      Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
      Microsoft::WRL::ComPtr<ID3D11Buffer> _constantBuffer;
      CrosshairConstantBuffer _constantBufferData;

      int _vertexCount = 0;
      bool _loadingComplete = false;
      Windows::Foundation::Numerics::float3 _position;
      Windows::Foundation::Numerics::float3 _color;
   };
}
