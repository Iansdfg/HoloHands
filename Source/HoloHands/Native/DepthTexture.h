#pragma once

#include "Native/Rendering/DeviceResources.h"

namespace HoloHands
{
   class DepthTexture : public DX::Resource
   {
      public:
         DepthTexture(std::shared_ptr<DX::DeviceResources> deviceResources);

         void CopyFromBitmap(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);

         void CreateDeviceDependentResources() override;
         void ReleaseDeviceDependentResources() override;

         ID3D11Texture2D* GetTexture(void) const { return m_texture.Get(); }         
         ID3D11ShaderResourceView* GetTextureView(void) const { return m_textureView.Get(); }

      private:
         Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
         Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureView;

         int m_width;
         int m_height;
   };
}