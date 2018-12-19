#pragma once

#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"
#include "ShaderStructures.h"

namespace HoloHands
{
   class DepthTexture;

   class QuadRenderer
   {
   public:
      QuadRenderer(
         const std::shared_ptr<DX::DeviceResources>& deviceResources,
         const Windows::Foundation::Size& size);

      void CreateDeviceDependentResources();
      void ReleaseDeviceDependentResources();
      void UpdatePosition(Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose);
      void Update(const DX::StepTimer& timer);
      void Render(const DepthTexture& depthTexture);

      Windows::Foundation::Numerics::float3 GetQuadPosition() { return m_quadPosition; }

   private:
      std::shared_ptr<DX::DeviceResources> m_deviceResources;

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
