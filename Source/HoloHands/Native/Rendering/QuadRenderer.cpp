#include "pch.h"

#include "QuadRenderer.h"

#include "Native/Rendering/DirectXHelper.h"
#include "Native/Rendering/DepthTexture.h"

using namespace HoloHands;
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Input::Spatial;
using namespace Microsoft::WRL;

QuadRenderer::QuadRenderer(
   const std::shared_ptr<DeviceResources>& deviceResources,
   const Size& size)
   :
   Resource(std::move(deviceResources)),
   m_quadPosition({ 0.f, 0.f, -2.f }),
   m_quadSize(size)
{
   CreateDeviceDependentResources();
}

void QuadRenderer::CreateDeviceDependentResources()
{
   task<std::vector<byte>> loadVSTask = ReadDataAsync(L"ms-appx:///Quad.vs.cso");
   task<std::vector<byte>> loadPSTask = ReadDataAsync(L"ms-appx:///Quad.ps.cso");

   task<void> createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
   {
      ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreateVertexShader(
            fileData.data(),
            fileData.size(),
            nullptr,
            &m_vertexShader
         )
      );

      constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> vertexDesc =
      { {
          { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
          { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      } };

      ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreateInputLayout(
            vertexDesc.data(),
            static_cast<UINT>(vertexDesc.size()),
            fileData.data(),
            static_cast<UINT>(fileData.size()),
            &m_inputLayout
         )
      );
   });

   task<void> createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
   {
      ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreatePixelShader(
            fileData.data(),
            fileData.size(),
            nullptr,
            &m_pixelShader
         )
      );

      const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
      ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_modelConstantBuffer
         )
      );
   });

   task<void> createTexture = concurrency::create_task([this]
   {
      D3D11_SAMPLER_DESC samplerDesc = {};
      samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
      samplerDesc.MinLOD = 0;
      samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

      ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc,
            m_sampler.ReleaseAndGetAddressOf()));
   });

   task<void> shaderTaskGroup = createPSTask && createVSTask && createTexture;
   task<void> createQuadTask = shaderTaskGroup.then([this]()
   {
      float aspect = m_quadSize.Width / m_quadSize.Height;

      float scale = 0.5f;
      float halfWidth = aspect * scale * 0.5f;
      float halfHeight = 1 * scale * 0.5f;

      static const std::array<VertexPositionTextureCoords, 4> quadVertices =
      { {
          { XMFLOAT3(-halfWidth, -halfHeight, 0.f), XMFLOAT2(0.f, 1.f) },
          { XMFLOAT3(-halfWidth, +halfHeight, 0.f), XMFLOAT2(0.f, 0.f) },
          { XMFLOAT3(+halfWidth, -halfHeight, 0.f), XMFLOAT2(1.f, 1.f) },
          { XMFLOAT3(+halfWidth, +halfHeight, 0.f), XMFLOAT2(1.f, 0.f) },
      } };

      m_vertexCount = static_cast<unsigned int>(quadVertices.size());

      D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
      vertexBufferData.pSysMem = quadVertices.data();
      vertexBufferData.SysMemPitch = 0;
      vertexBufferData.SysMemSlicePitch = 0;

      const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionTextureCoords) * static_cast<UINT>(quadVertices.size()), D3D11_BIND_VERTEX_BUFFER);

      ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_vertexBuffer
         )
      );
   });

   createQuadTask.then([this]()
   {
      m_loadingComplete = true;
   });
}

void QuadRenderer::ReleaseDeviceDependentResources()
{
   m_loadingComplete = false;
   m_vertexShader.Reset();
   m_inputLayout.Reset();
   m_pixelShader.Reset();
   m_modelConstantBuffer.Reset();
   m_vertexBuffer.Reset();
}

void QuadRenderer::UpdatePosition(SpatialPointerPose^ pointerPose)
{
   if (pointerPose != nullptr)
   {
      m_headPosition = pointerPose->Head->Position;
      m_headForwardDirection = pointerPose->Head->ForwardDirection;
      m_headUpDirection = pointerPose->Head->UpDirection;

      float distance = 2;
      m_quadPosition = m_headPosition + m_headForwardDirection * distance;
   }
}

void QuadRenderer::Update(const StepTimer& timer)
{
   const XMMATRIX rotation = XMMatrixTranspose(XMMatrixLookAtRH(
      XMLoadFloat3(&m_headPosition),
      XMLoadFloat3(&m_quadPosition),
      XMLoadFloat3(&m_headUpDirection)));

   const XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&m_quadPosition));

   XMStoreFloat4x4(&m_modelConstantBufferData.model, XMMatrixTranspose(rotation * translation));

   if (!m_loadingComplete)
   {
      return;
   }

   const auto context = m_deviceResources->GetD3DDeviceContext();

   context->UpdateSubresource(
      m_modelConstantBuffer.Get(),
      0,
      nullptr,
      &m_modelConstantBufferData,
      0,
      0
   );
}

void QuadRenderer::Render(const DepthTexture& depthTexture)
{
   if (!m_loadingComplete)
   {
      return;
   }

   const auto context = m_deviceResources->GetD3DDeviceContext();

   const UINT stride = sizeof(VertexPositionTextureCoords);
   const UINT offset = 0;
   context->IASetVertexBuffers(
      0,
      1,
      m_vertexBuffer.GetAddressOf(),
      &stride,
      &offset
   );

   context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
   context->IASetInputLayout(m_inputLayout.Get());

   context->VSSetShader(
      m_vertexShader.Get(),
      nullptr,
      0
   );

   context->VSSetConstantBuffers(
      0,
      1,
      m_modelConstantBuffer.GetAddressOf()
   );

   context->PSSetShader(
      m_pixelShader.Get(),
      nullptr,
      0
   );

   context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

   auto texture = depthTexture.GetTextureView();
   if (texture != nullptr)
   {
      context->PSSetShaderResources(0, 1, &texture);
   }

   context->DrawInstanced(m_vertexCount, 2, 0, 0);
}

std::vector<uint8_t> LoadBGRAImage(const wchar_t* filename, uint32_t& width, uint32_t& height)
{
   ComPtr<IWICImagingFactory> wicFactory;
   ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)));

   ComPtr<IWICBitmapDecoder> decoder;
   ThrowIfFailed(wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf()));

   ComPtr<IWICBitmapFrameDecode> frame;
   ThrowIfFailed(decoder->GetFrame(0, frame.GetAddressOf()));

   ThrowIfFailed(frame->GetSize(&width, &height));

   WICPixelFormatGUID pixelFormat;
   ThrowIfFailed(frame->GetPixelFormat(&pixelFormat));

   uint32_t rowPitch = width * sizeof(uint32_t);
   uint32_t imageSize = rowPitch * height;

   std::vector<uint8_t> image;
   image.resize(size_t(imageSize));

   if (memcmp(&pixelFormat, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID)) == 0)
   {
      ThrowIfFailed(frame->CopyPixels(nullptr, rowPitch, imageSize, reinterpret_cast<BYTE*>(image.data())));
   }
   else
   {
      ComPtr<IWICFormatConverter> formatConverter;
      ThrowIfFailed(wicFactory->CreateFormatConverter(formatConverter.GetAddressOf()));

      BOOL canConvert = FALSE;
      ThrowIfFailed(formatConverter->CanConvert(pixelFormat, GUID_WICPixelFormat32bppBGRA, &canConvert));
      if (!canConvert)
      {
         throw std::exception("CanConvert");
      }

      ThrowIfFailed(formatConverter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
         WICBitmapDitherTypeErrorDiffusion, nullptr, 0, WICBitmapPaletteTypeMedianCut));

      ThrowIfFailed(formatConverter->CopyPixels(nullptr, rowPitch, imageSize, reinterpret_cast<BYTE*>(image.data())));
   }

   return image;
}
