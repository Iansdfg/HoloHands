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

      Windows::Foundation::Numerics::float3 GetQuadPosition() { return m_quadPosition; }

   private:
      Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
      Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
      Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
      Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
      Microsoft::WRL::ComPtr<ID3D11Buffer> m_modelConstantBuffer;

      ModelConstantBuffer m_modelConstantBufferData;
      uint32 m_vertexCount = 0;

      bool m_loadingComplete = false;

      Windows::Foundation::Size m_quadSize;
      Windows::Foundation::Numerics::float3 m_quadPosition;
      Windows::Foundation::Numerics::float3 m_headPosition;
      Windows::Foundation::Numerics::float3 m_headForwardDirection;
      Windows::Foundation::Numerics::float3 m_headUpDirection;

      Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
      Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
   };
}
