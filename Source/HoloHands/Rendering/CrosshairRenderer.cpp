#include "pch.h"

#include "CrosshairRenderer.h"

using namespace HoloHands;
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Input::Spatial;
using namespace Microsoft::WRL;

namespace HoloHands
{
   CrosshairRenderer::CrosshairRenderer(const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
      :
      _deviceResources(deviceResources),
      _position({ 0, 0, 0 }),
      _color({ 1, 1, 1 }),
      _vertexCount(0),
      _loadingComplete(false)
   {
      CreateDeviceDependentResources();
   }

   void CrosshairRenderer::CreateDeviceDependentResources()
   {
      task<std::vector<byte>> loadVSTask = Io::ReadDataAsync(L"ms-appx:///Basic.vs.cso");
      task<std::vector<byte>> loadPSTask = Io::ReadDataAsync(L"ms-appx:///Basic.ps.cso");

      task<void> createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
      {
         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreateVertexShader(
               fileData.data(),
               fileData.size(),
               nullptr,
               &_vertexShader
            )
         );

         constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1> vertexDesc =
         { {
             { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
         } };

         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreateInputLayout(
               vertexDesc.data(),
               static_cast<UINT>(vertexDesc.size()),
               fileData.data(),
               static_cast<UINT>(fileData.size()),
               &_inputLayout
            )
         );
      });

      task<void> createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
      {
         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreatePixelShader(
               fileData.data(),
               fileData.size(),
               nullptr,
               &_pixelShader
            )
         );

         const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(CrosshairConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreateBuffer(
               &constantBufferDesc,
               nullptr,
               &_constantBuffer
            )
         );
      });

      task<void> shaderTaskGroup = createPSTask && createVSTask;
      task<void> createQuadTask = shaderTaskGroup.then([this]()
      {
         const float l = 0.03f;
         static const std::array<VertexPosition, 12> cubeVertices =
         {
            {
               { {-l, 0, 0 } },
               { { 0, 0, 0 } },
               { { 0, 0, 0 } },
               { { l, 0, 0 } },
               { { 0,-l, 0 } },
               { { 0, 0, 0 } },
               { { 0, 0, 0 } },
               { { 0, l, 0 } },
               { { 0, 0,-l } },
               { { 0, 0, 0 } },
               { { 0, 0, 0 } },
               { { 0, 0, l } },
            }
         };

         _vertexCount = cubeVertices.size();

         D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };

         vertexBufferData.pSysMem = cubeVertices.data();
         vertexBufferData.SysMemPitch = 0;
         vertexBufferData.SysMemSlicePitch = 0;

         const CD3D11_BUFFER_DESC vertexBufferDesc(
            static_cast<uint32_t>(sizeof(VertexPosition) * cubeVertices.size()),
            D3D11_BIND_VERTEX_BUFFER);

         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreateBuffer(
               &vertexBufferDesc,
               &vertexBufferData,
               &_vertexBuffer
            )
         );
      });

      createQuadTask.then([this]()
      {
         _loadingComplete = true;
      });
   }

   void CrosshairRenderer::ReleaseDeviceDependentResources()
   {
      _loadingComplete = false;

      _vertexShader.Reset();
      _inputLayout.Reset();
      _pixelShader.Reset();
      _constantBuffer.Reset();
      _vertexBuffer.Reset();
   }

   void CrosshairRenderer::Update()
   {
      //Update tranforms.
      const DirectX::XMMATRIX modelTransform =
         DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&_position));

      XMStoreFloat4x4(&_constantBufferData.model, DirectX::XMMatrixTranspose(modelTransform));

      //Update color.
      float4 color(_color, 1);
      XMStoreFloat4(&_constantBufferData.color, DirectX::XMLoadFloat4(&color));

      if (!_loadingComplete)
      {
         return;
      }

      const auto context = _deviceResources->GetD3DDeviceContext();

      context->UpdateSubresource(
         _constantBuffer.Get(),
         0,
         nullptr,
         &_constantBufferData,
         0,
         0
      );
   }

   void CrosshairRenderer::Render()
   {
      if (!_loadingComplete)
      {
         return;
      }

      const auto context = _deviceResources->GetD3DDeviceContext();

      //Bind shaders.
      context->VSSetShader(
         _vertexShader.Get(),
         nullptr,
         0
      );

      context->PSSetShader(
         _pixelShader.Get(),
         nullptr,
         0
      );

      context->IASetInputLayout(_inputLayout.Get());

      const UINT stride = sizeof(VertexPosition);
      const UINT offset = 0;
      context->IASetVertexBuffers(
         0,
         1,
         _vertexBuffer.GetAddressOf(),
         &stride,
         &offset
      );

      context->VSSetConstantBuffers(
         0,
         1,
         _constantBuffer.GetAddressOf()
      );

      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

      context->DrawInstanced(_vertexCount, 2, 0, 0);
   }

   void CrosshairRenderer::SetPosition(const float3& position)
   {
      _position = position;
   }

   void CrosshairRenderer::SetColor(const float3& color)
   {
      _color = color;
   }
}
