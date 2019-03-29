#pragma once

namespace HoloHands
{
   class DepthTexture
   {
   public:
      DepthTexture(
         const std::shared_ptr<Graphics::DeviceResources>& deviceResources);

      //Copy a SoftwareBitmap into a DirectX texture.
      //Only supports Gray 16 bit images.
      void CopyFrom(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);

      //Copy an OpenCV Mat bitmap into a DirectX texture.
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