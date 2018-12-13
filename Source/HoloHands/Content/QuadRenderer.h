#pragma once

#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"
#include "ShaderStructures.h"

namespace HoloHands
{
   class QuadRenderer
   {
   public:
      QuadRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
      void CreateDeviceDependentResources();
      void ReleaseDeviceDependentResources();
      void Update(const DX::StepTimer& timer);
      void Render();

   private:
      std::shared_ptr<DX::DeviceResources>            m_deviceResources;

      Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
      Microsoft::WRL::ComPtr<ID3D11Buffer>            m_vertexBuffer;
      Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vertexShader;
      Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_pixelShader;
      Microsoft::WRL::ComPtr<ID3D11Buffer>            m_modelConstantBuffer;

      ModelConstantBuffer                             m_modelConstantBufferData;
      uint32                                          m_vertexCount = 0;

      bool                                            m_loadingComplete = false;
   };
}
