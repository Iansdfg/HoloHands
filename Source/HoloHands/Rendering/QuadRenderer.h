#pragma once

#include "Native/Rendering/DeviceResources.h"
#include "Native/Rendering/ShaderStructures.h"
#include "Native/Utils/StepTimer.h"

namespace HoloHands
{
   class DepthTexture;

   class QuadRenderer : public Resource
   {
   public:
      QuadRenderer(
         const std::shared_ptr<DeviceResources>& deviceResources,
         const Windows::Foundation::Size& size);

      void CreateDeviceDependentResources() override;
      void ReleaseDeviceDependentResources() override;

      void UpdatePosition(Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose);
      void Update(const StepTimer& timer);
      void Render(const DepthTexture& depthTexture);

      Windows::Foundation::Numerics::float3 GetQuadPosition() { return _quadPosition; }

   private:
      Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
      Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
      Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
      Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;
      Microsoft::WRL::ComPtr<ID3D11Buffer> _modelConstantBuffer;

      ModelConstantBuffer _modelConstantBufferData;
      uint32 _vertexCount = 0;

      bool _loadingComplete = false;

      Windows::Foundation::Size _quadSize;
      Windows::Foundation::Numerics::float3 _quadPosition;
      Windows::Foundation::Numerics::float3 _headPosition;
      Windows::Foundation::Numerics::float3 _headForwardDirection;
      Windows::Foundation::Numerics::float3 _headUpDirection;

      Microsoft::WRL::ComPtr<ID3D11SamplerState> _sampler;
      Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _texture;
   };
}
