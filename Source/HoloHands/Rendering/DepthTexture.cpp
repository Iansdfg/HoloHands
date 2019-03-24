#include "pch.h"

#include "DepthTexture.h"

#include <MemoryBuffer.h>

using namespace HoloHands;
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Graphics::Imaging;
using namespace Microsoft::WRL;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Media::Imaging;

DepthTexture::DepthTexture(
   const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
   :
   _deviceResources(deviceResources),
   _width(1),
   _height(1)
{
}

void DepthTexture::CopyFrom(SoftwareBitmap^ bitmap)
{
   if (_texture == nullptr || bitmap == nullptr)
   {
      return;
   }

   int width = bitmap->PixelWidth;
   int height = bitmap->PixelHeight;

   if (width != _width || height != _height)
   {
      _width = width;
      _height = height;

      ReleaseDeviceDependentResources();
      CreateDeviceDependentResources();
   }

   auto format = bitmap->BitmapPixelFormat;

   if (format != BitmapPixelFormat::Gray16)
   {
      OutputDebugString(L"BitmapPixelFormat was not depth\r\n");
      return;
   }

   BitmapBuffer^ bitmapBuffer = bitmap->LockBuffer(BitmapBufferAccessMode::Read);
   IMemoryBufferReference^ bufferRef = bitmapBuffer->CreateReference();

   ComPtr<IMemoryBufferByteAccess> memoryBufferByteAccess;
   if (SUCCEEDED(reinterpret_cast<IInspectable*>(bufferRef)->QueryInterface(IID_PPV_ARGS(&memoryBufferByteAccess))))
   {
      BYTE* pSourceBuffer = nullptr;
      UINT32 sourceCapacity = 0;
      if (SUCCEEDED(memoryBufferByteAccess->GetBuffer(&pSourceBuffer, &sourceCapacity)) && pSourceBuffer)
      {
         auto const context = _deviceResources->GetD3DDeviceContext();

         D3D11_MAPPED_SUBRESOURCE subResource;
         if (SUCCEEDED(context->Map(_texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource)))
         {
            std::memcpy(subResource.pData, pSourceBuffer, sourceCapacity);
            context->Unmap(_texture.Get(), 0);
         }
      }
   }
}

void DepthTexture::CopyFrom(cv::Mat& matrix)
{
   //matrix.setTo(cv::Scalar(255)); //TODO: temp

   int width = matrix.cols;
   int height = matrix.rows;

   if (width != _width || height != _height)
   {
      _width = width;
      _height = height;

      ReleaseDeviceDependentResources();
      CreateDeviceDependentResources();
   }

   auto const context = _deviceResources->GetD3DDeviceContext();

   D3D11_MAPPED_SUBRESOURCE subResource;
   if (SUCCEEDED(context->Map(_texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource)))
   {
      std::memcpy(subResource.pData, matrix.data, matrix.total() * matrix.elemSize());
      context->Unmap(_texture.Get(), 0);
   }
}

void DepthTexture::CreateDeviceDependentResources()
{
   D3D11_TEXTURE2D_DESC const texDesc = CD3D11_TEXTURE2D_DESC(
      DXGI_FORMAT_R8_UNORM, //DXGI_FORMAT_R16_UNORM
      _width,
      _height,
      1,
      1,
      D3D11_BIND_SHADER_RESOURCE,
      D3D11_USAGE_DYNAMIC,
      D3D11_CPU_ACCESS_WRITE
   );

   ASSERT_SUCCEEDED(
      _deviceResources->GetD3DDevice()->CreateTexture2D(
         &texDesc,
         nullptr,
         &_texture
      )
   );

   D3D11_SHADER_RESOURCE_VIEW_DESC const viewDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
      _texture.Get(),
      D3D11_SRV_DIMENSION_TEXTURE2D,
      DXGI_FORMAT_R8_UNORM); //DXGI_FORMAT_R16_UNORM

   ASSERT_SUCCEEDED(
      _deviceResources->GetD3DDevice()->CreateShaderResourceView(
         _texture.Get(),
         &viewDesc,
         _textureView.ReleaseAndGetAddressOf()));
}

void DepthTexture::ReleaseDeviceDependentResources()
{
   if (_texture != nullptr)
   {
      _texture.Reset();
   }
}

ID3D11Texture2D* DepthTexture::GetTexture(void) const
{
   return _texture.Get();
}

ID3D11ShaderResourceView* DepthTexture::GetTextureView(void) const
{
   return _textureView.Get();
}

