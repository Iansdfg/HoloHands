#include "pch.h"
#include "QuadRenderer.h"
#include "Common\DirectXHelper.h"

using namespace HoloHands;
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Input::Spatial;

QuadRenderer::QuadRenderer(
   const std::shared_ptr<DX::DeviceResources>& deviceResources,
   const Size& size)
   :
   m_deviceResources(deviceResources),
   m_quadPosition({ 0.f, 0.f, -2.f }),
   m_quadSize(size)
{
   CreateDeviceDependentResources();
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

void QuadRenderer::Update(const DX::StepTimer& timer)
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


void QuadRenderer::Render()
{
   if (!m_loadingComplete)
   {
      return;
   }

   const auto context = m_deviceResources->GetD3DDeviceContext();

   const UINT stride = sizeof(VertexPositionColor);
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

   context->DrawInstanced(
      m_vertexCount,   // Index count per instance.
      2,              // Instance count.
      0,              // Base vertex location.
      0               // Start instance location.
   );
}

void QuadRenderer::CreateDeviceDependentResources()
{
   task<std::vector<byte>> loadVSTask = DX::ReadDataAsync(L"ms-appx:///Quad.vs.cso");
   task<std::vector<byte>> loadPSTask = DX::ReadDataAsync(L"ms-appx:///Quad.ps.cso");

   // After the vertex shader file is loaded, create the shader and input layout.
   task<void> createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
   {
      DX::ThrowIfFailed(
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
          { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      } };

      DX::ThrowIfFailed(
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
      DX::ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreatePixelShader(
            fileData.data(),
            fileData.size(),
            nullptr,
            &m_pixelShader
         )
      );

      const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
      DX::ThrowIfFailed(
         m_deviceResources->GetD3DDevice()->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_modelConstantBuffer
         )
      );
   });

   task<void> shaderTaskGroup = createPSTask && createVSTask;
   task<void> createQuadTask = shaderTaskGroup.then([this]()
   {
      float aspect = m_quadSize.Width / m_quadSize.Height;

      float scale = 0.5f;
      float halfWidth = aspect * scale * 0.5f;
      float halfHeight = 1 * scale * 0.5f;

      static const std::array<VertexPositionColor, 4> quadVertices =
      { {
          { XMFLOAT3(-halfWidth, -halfHeight, 0.f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
          { XMFLOAT3(-halfWidth, +halfHeight, 0.f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
          { XMFLOAT3(+halfWidth, -halfHeight, 0.f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
          { XMFLOAT3(+halfWidth, +halfHeight, 0.f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
      } };

      m_vertexCount = static_cast<unsigned int>(quadVertices.size());

      D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
      vertexBufferData.pSysMem = quadVertices.data();
      vertexBufferData.SysMemPitch = 0;
      vertexBufferData.SysMemSlicePitch = 0;

      const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionColor) * static_cast<UINT>(quadVertices.size()), D3D11_BIND_VERTEX_BUFFER);

      DX::ThrowIfFailed(
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
