#pragma once

namespace HoloHands
{
   class DepthTexture
   {
   public:
      DepthTexture(
         const std::shared_ptr<Graphics::DeviceResources>& deviceResources);

      void CopyFrom(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);
      void CopyFrom(cv::Mat& matrix);

      void CreateDeviceDependentResources();
      void ReleaseDeviceDependentResources();

      ID3D11Texture2D* GetTexture(void) const;
      ID3D11ShaderResourceView* GetTextureView(void) const;

   private:
      Graphics::DeviceResourcesPtr _deviceResources;

      Microsoft::WRL::ComPtr<ID3D11Texture2D> _texture;
      Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _textureView;

      int _width;
      int _height;
   };
}