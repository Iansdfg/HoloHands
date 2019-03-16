#include "pch.h"

#include "CubeRenderer.h"

namespace HoloHands
{
   CubeRenderer::CubeRenderer(
      const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
      : _deviceResources(deviceResources)
   {
      CreateDeviceDependentResources();
   }

   void CubeRenderer::PositionHologram(
      Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose)
   {
      if (pointerPose != nullptr)
      {
         using Windows::Foundation::Numerics::float3;

         const float3 headPosition = pointerPose->Head->Position;
         const float3 headDirection = pointerPose->Head->ForwardDirection;

         constexpr float distanceFromUser = 2.0f;
         const float3 gazeAtTwoMeters = headPosition + (distanceFromUser * headDirection);

         SetPosition(gazeAtTwoMeters);

         _rotationInRadians = std::atan2(
            headDirection.z,
            headDirection.x) + DirectX::XM_PIDIV2;
      }
   }

   void CubeRenderer::Update(_In_ const Graphics::StepTimer&)
   {
      const DirectX::XMMATRIX modelTranslation = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&_position));

      XMStoreFloat4x4(&_modelConstantBufferData.model, DirectX::XMMatrixTranspose(modelTranslation));

      if (!_loadingComplete)
      {
         return;
      }

      const auto context = _deviceResources->GetD3DDeviceContext();

      context->UpdateSubresource(
         _modelConstantBuffer.Get(),
         0,
         nullptr,
         &_modelConstantBufferData,
         0,
         0);
   }

   void CubeRenderer::Render()
   {
      if (!_loadingComplete)
      {
         return;
      }

      const auto context = _deviceResources->GetD3DDeviceContext();

      _slateMaterial->Bind();

      const UINT stride = sizeof(Rendering::VertexPositionColorTexture);
      const UINT offset = 0;
      context->IASetVertexBuffers(
         0,
         1,
         _vertexBuffer.GetAddressOf(),
         &stride,
         &offset
      );
      context->IASetIndexBuffer(
         _indexBuffer.Get(),
         DXGI_FORMAT_R16_UINT,
         0
      );

      context->VSSetConstantBuffers(
         0,
         1,
         _modelConstantBuffer.GetAddressOf()
      );

      ID3D11ShaderResourceView* shaderResourceViews[1] = { nullptr };
      context->PSSetShaderResources(0, 1, shaderResourceViews);

      context->DrawIndexedInstanced(_indexCount, 2, 0, 0, 0);
   }

   void CubeRenderer::CreateDeviceDependentResources()
   {
      if (nullptr == _slateMaterial)
      {
         _slateMaterial =
            std::make_unique<Rendering::SlateMaterial>(
               _deviceResources);
      }

      _slateMaterial->CreateDeviceDependentResources();

      {
         const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(Rendering::SlateModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreateBuffer(
               &constantBufferDesc,
               nullptr,
               &_modelConstantBuffer
            )
         );
      }

      // Once all shaders are loaded, create the mesh.
      {
         // Load mesh vertices. Each vertex has a position and a color.
         // Note that the cube size has changed from the default DirectX app
         // template. Windows Holographic is scaled in meters, so to draw the
         // cube at a comfortable size we made the cube width 0.2 m (20 cm).
         const float sx = 0.0035f, sy = 0.0035f, sz = 0.0035f;
         static const std::array<Rendering::VertexPositionColorTexture, 8> cubeVertices =
         { {
             { { -sx, -sy, -sz }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
             { { -sx, -sy,  sz }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
             { { -sx,  sy, -sz }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
             { { -sx,  sy,  sz }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
             { {  sx, -sy, -sz }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
             { {  sx, -sy,  sz }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
             { {  sx,  sy, -sz }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
             { {  sx,  sy,  sz }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
         } };

         D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };

         vertexBufferData.pSysMem = cubeVertices.data();
         vertexBufferData.SysMemPitch = 0;
         vertexBufferData.SysMemSlicePitch = 0;

         const CD3D11_BUFFER_DESC vertexBufferDesc(
            static_cast<uint32_t>(sizeof(Rendering::VertexPositionColorTexture) * cubeVertices.size()),
            D3D11_BIND_VERTEX_BUFFER);

         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreateBuffer(
               &vertexBufferDesc,
               &vertexBufferData,
               &_vertexBuffer
            )
         );

         // Load mesh indices. Each trio of indices represents
         // a triangle to be rendered on the screen.
         // For example: 2,1,0 means that the vertices with indexes
         // 2, 1, and 0 from the vertex buffer compose the
         // first triangle of this mesh.
         // Note that the winding order is clockwise by default.
         constexpr std::array<unsigned short, 36> cubeIndices =
         { {
                 2,1,0, // -x
                 2,3,1,

                 6,4,5, // +x
                 6,5,7,

                 0,1,5, // -y
                 0,5,4,

                 2,6,7, // +y
                 2,7,3,

                 0,4,6, // -z
                 0,6,2,

                 1,3,7, // +z
                 1,7,5,
             } };

         _indexCount = static_cast<uint32_t>(cubeIndices.size());

         D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };

         indexBufferData.pSysMem = cubeIndices.data();
         indexBufferData.SysMemPitch = 0;
         indexBufferData.SysMemSlicePitch = 0;

         CD3D11_BUFFER_DESC indexBufferDesc(
            static_cast<uint32_t>(sizeof(unsigned short) * cubeIndices.size()),
            D3D11_BIND_INDEX_BUFFER);

         ASSERT_SUCCEEDED(
            _deviceResources->GetD3DDevice()->CreateBuffer(
               &indexBufferDesc,
               &indexBufferData,
               &_indexBuffer
            )
         );
      }

      _loadingComplete = true;
   }

   void CubeRenderer::ReleaseDeviceDependentResources()
   {
      _loadingComplete = false;

      _modelConstantBuffer.Reset();
      _vertexBuffer.Reset();
      _indexBuffer.Reset();

      if (nullptr != _slateMaterial)
      {
         _slateMaterial->ReleaseDeviceDependentResources();
      }
   }
}
