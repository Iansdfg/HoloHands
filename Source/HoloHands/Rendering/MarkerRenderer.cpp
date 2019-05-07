#include "pch.h"

#include "MarkerRenderer.h"

using namespace HoloHands;
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Input::Spatial;
using namespace Microsoft::WRL;

namespace HoloHands
{
   MarkerRenderer::MarkerRenderer(const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
      :
      _deviceResources(deviceResources),
      _direction({ 1, 0, 0 }),
      _color({1, 0, 0}),
      _vertexCount(0),
      _loadingComplete(false)
   {
      CreateDeviceDependentResources();
   }

   void MarkerRenderer::CreateDeviceDependentResources()
   {
      task<std::vector<byte>> loadVSTask = Io::ReadDataAsync(L"ms-appx:///Basic.vs.cso");
      task<std::vector<byte>> loadPSTask = Io::ReadDataAsync(L"ms-appx:///Basic.ps.cso");

      //Create vertex shader.
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

      //Create pixel shader.
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

      //Create vertex data.
      task<void> createQuadTask = shaderTaskGroup.then([this]()
      {
         static const std::array<VertexPosition, 2> vertices =
         {
            {
               { { 0, 0, 0 } },
               { { 0, 1, 0 } },
            }
         };

         _vertexCount = vertices.size();

         D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };

         vertexBufferData.pSysMem = vertices.data();
         vertexBufferData.SysMemPitch = 0;
         vertexBufferData.SysMemSlicePitch = 0;

         const CD3D11_BUFFER_DESC vertexBufferDesc(
            static_cast<uint32_t>(sizeof(VertexPosition) * vertices.size()),
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

   void MarkerRenderer::ReleaseDeviceDependentResources()
   {
      _loadingComplete = false;

      _vertexShader.Reset();
      _inputLayout.Reset();
      _pixelShader.Reset();
      _constantBuffer.Reset();
      _vertexBuffer.Reset();
   }

   void MarkerRenderer::Update(SpatialPointerPose^ pose)
   {
      if (pose == nullptr)
      {
         return;
      }

      auto headForwardDirection = pose->Head->ForwardDirection;
      auto headUpDirection = pose->Head->UpDirection;
      auto position = pose->Head->Position + headForwardDirection * 4.0;

      const float3 quadNormal(0, 0, -1);
      float3 forwardDirection(headForwardDirection.x, 0, headForwardDirection.z);

      //Rotate quad to always face the view.
      XMVECTOR angle = XMVector3AngleBetweenVectors(XMLoadFloat3(&forwardDirection), XMLoadFloat3(&quadNormal));
      auto downDirection = cross(forwardDirection, quadNormal);

      if (dot(downDirection, headUpDirection) > 0)
      {
         //Invert angle when facing the other direction.
         float3 tempAngle;
         XMStoreFloat3(&tempAngle, angle);
         tempAngle *= -1;
         angle = XMLoadFloat3(&tempAngle);
      }

      //Create transforms.
      const XMMATRIX rotationY = XMMatrixRotationY(XMVectorGetY(angle));

      const float deg = 45.0f;
      const float PI = 3.1415926;
      const XMMATRIX rotationZ = XMMatrixRotationZ(deg * PI / 180.f);
      const XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&position));

      //Store model transform.
      XMStoreFloat4x4(&_constantBufferData.model, XMMatrixTranspose(rotationZ * rotationY * translation));

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

   void MarkerRenderer::Render()
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

      //Bind buffers.
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

      //Draw.
      context->DrawInstanced(_vertexCount, 2, 0, 0);
   }

   void MarkerRenderer::SetDirection(const float3& direction)
   {
      _direction = direction;
   }
}
