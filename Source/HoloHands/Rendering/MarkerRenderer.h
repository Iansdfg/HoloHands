#pragma once

#include "Rendering/Shaders/ShaderStructs.h"

namespace HoloHands
{
   class MarkerRenderer
   {
   public:
      MarkerRenderer(const std::shared_ptr<Graphics::DeviceResources>& deviceResources);
      void CreateDeviceDependentResources();
      void ReleaseDeviceDependentResources();

      void UpdatePose(Windows::UI::Input::Spatial::SpatialPointerPose^ pose);
      void SetPosition(const Windows::Foundation::Numerics::float3& targetPosition);
      void Update();
      void Render();

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
      Windows::Foundation::Numerics::float3 _direction;
      Windows::Foundation::Numerics::float3 _color;
      Windows::Foundation::Numerics::float3 _headPosition;
      Windows::Foundation::Numerics::float3 _headForwardDirection;
      Windows::Foundation::Numerics::float3 _headUpDirection;
      DirectX::XMMATRIX _rotationZ;
   };
}
