#include "pch.h"

#include "DepthTexture.h"

#include "Native/Rendering/DirectXHelper.h"

#include "Io/All.h"
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

DepthTexture::DepthTexture(std::shared_ptr<DeviceResources> deviceResources)
   :
   Resource(std::move(deviceResources)),
   m_width(1),
   m_height(1)
{
}

void DepthTexture::CopyFrom(SoftwareBitmap^ bitmap)
{
   if (m_texture == nullptr || bitmap == nullptr)
   {
      return;
   }

   int width = bitmap->PixelWidth;
   int height = bitmap->PixelHeight;

   if (width != m_width || height != m_height)
   {
      m_width = width;
      m_height = height;

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
         auto const context = m_deviceResources->GetD3DDeviceContext();

         D3D11_MAPPED_SUBRESOURCE subResource;
         if (SUCCEEDED(context->Map(m_texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource)))
         {
            std::memcpy(subResource.pData, pSourceBuffer, sourceCapacity);
            context->Unmap(m_texture.Get(), 0);
         }
      }
   }
}

void DepthTexture::CopyFrom(cv::Mat& matrix)
{
   if (m_texture == nullptr)
   {
      return;
   }

   int width = matrix.cols;
   int height = matrix.rows;

   if (width != m_width || height != m_height)
   {
      m_width = width;
      m_height = height;

      ReleaseDeviceDependentResources();
      CreateDeviceDependentResources();
   }

   auto const context = m_deviceResources->GetD3DDeviceContext();

   D3D11_MAPPED_SUBRESOURCE subResource;
   if (SUCCEEDED(context->Map(m_texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource)))
   {
      std::memcpy(subResource.pData, matrix.data, matrix.total() * matrix.elemSize());
      context->Unmap(m_texture.Get(), 0);
   }
}

void DepthTexture::CreateDeviceDependentResources()
{
   D3D11_TEXTURE2D_DESC const texDesc = CD3D11_TEXTURE2D_DESC(
      DXGI_FORMAT_R8_UNORM, //DXGI_FORMAT_R16_UNORM
      m_width,                    // Width of the video frames
      m_height,                   // Height of the video frames
      1,                          // Number of textures in the array
      1,                          // Number of miplevels in each texture
      D3D11_BIND_SHADER_RESOURCE, // We read from this texture in the shader
      D3D11_USAGE_DYNAMIC,        // Because we'll be copying from CPU memory
      D3D11_CPU_ACCESS_WRITE      // We only need to write into the texture
   );

   ThrowIfFailed(
      m_deviceResources->GetD3DDevice()->CreateTexture2D(
         &texDesc,
         nullptr,
         &m_texture
      )
   );

   D3D11_SHADER_RESOURCE_VIEW_DESC const viewDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
      m_texture.Get(),
      D3D11_SRV_DIMENSION_TEXTURE2D,
      DXGI_FORMAT_R8_UNORM); //DXGI_FORMAT_R16_UNORM

   ThrowIfFailed(
      m_deviceResources->GetD3DDevice()->CreateShaderResourceView(
         m_texture.Get(),
         &viewDesc,
         m_textureView.ReleaseAndGetAddressOf()));
}

void DepthTexture::ReleaseDeviceDependentResources()
{
   if (m_texture != nullptr)
   {
      m_texture.Reset();
   }
}

ID3D11Texture2D* DepthTexture::GetTexture(void) const
{
   return m_texture.Get();
}

ID3D11ShaderResourceView* DepthTexture::GetTextureView(void) const
{
   return m_textureView.Get();
}

