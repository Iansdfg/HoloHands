#include "pch.h"
#include "QuadRenderer.h"
#include "Common\DirectXHelper.h"

using namespace HoloHands;
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Input::Spatial;

QuadRenderer::QuadRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources)
   :
   m_deviceResources(deviceResources)
{
   CreateDeviceDependentResources();
}

void QuadRenderer::Update(const DX::StepTimer& timer)
{
   Windows::Foundation::Numerics::float3 position = { 0.f, 0.f, -2.f };
   const XMMATRIX modelTransform = XMMatrixTranslationFromVector(XMLoadFloat3(&position));

   XMStoreFloat4x4(&m_modelConstantBufferData.model, XMMatrixTranspose(modelTransform));

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
      static const std::array<VertexPositionColor, 4> triangleVertices =
      { {
          { XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
          { XMFLOAT3(-0.5f, +0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
          { XMFLOAT3(+0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
          { XMFLOAT3(+0.5f, +0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
      } };

      m_vertexCount = static_cast<unsigned int>(triangleVertices.size());

      D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
      vertexBufferData.pSysMem = triangleVertices.data();
      vertexBufferData.SysMemPitch = 0;
      vertexBufferData.SysMemSlicePitch = 0;

      const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionColor) * static_cast<UINT>(triangleVertices.size()), D3D11_BIND_VERTEX_BUFFER);

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
