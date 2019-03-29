#include "pch.h"

#include "QuadRenderer.h"

#include "Rendering/DepthTexture.h"

using namespace HoloHands;
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Input::Spatial;
using namespace Microsoft::WRL;

QuadRenderer::QuadRenderer(
   const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
   :
   _deviceResources(deviceResources),
   _quadPosition({ 0.f, 0.f, 0.f }),
   _quadSize(Windows::Foundation::Size(1.4f, 1.f)),
   _quadOffset({ -0.6f, 0.3f, 4.0f })
{
   CreateDeviceDependentResources();
}

void QuadRenderer::CreateDeviceDependentResources()
{
   task<std::vector<byte>> loadVSTask = Io::ReadDataAsync(L"ms-appx:///Quad.vs.cso");
   task<std::vector<byte>> loadPSTask = Io::ReadDataAsync(L"ms-appx:///Quad.ps.cso");

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

      constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> vertexDesc =
      { {
          { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
          { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

      const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
      ASSERT_SUCCEEDED(
         _deviceResources->GetD3DDevice()->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &_modelConstantBuffer
         )
      );
   });

   //Create texture.
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

      ASSERT_SUCCEEDED(
         _deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc,
            _sampler.ReleaseAndGetAddressOf()));
   });

   task<void> shaderTaskGroup = createPSTask && createVSTask && createTexture;

   //Create vertex data.
   task<void> createQuadTask = shaderTaskGroup.then([this]()
   {
      float aspect = _quadSize.Width / _quadSize.Height;

      float scale = 0.5f;
      float halfWidth = aspect * scale * 0.5f;
      float halfHeight = 1 * scale * 0.5f;

      static const std::array<VertexPositionTextureCoords, 4> quadVertices =
      { {
          { XMFLOAT3(-halfWidth + _quadOffset.x, -halfHeight + _quadOffset.y, 0.f), XMFLOAT2(0.f, 1.f) },
          { XMFLOAT3(-halfWidth + _quadOffset.x, +halfHeight + _quadOffset.y, 0.f), XMFLOAT2(0.f, 0.f) },
          { XMFLOAT3(+halfWidth + _quadOffset.x, -halfHeight + _quadOffset.y, 0.f), XMFLOAT2(1.f, 1.f) },
          { XMFLOAT3(+halfWidth + _quadOffset.x, +halfHeight + _quadOffset.y, 0.f), XMFLOAT2(1.f, 0.f) },
      } };

      _vertexCount = static_cast<unsigned int>(quadVertices.size());

      D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
      vertexBufferData.pSysMem = quadVertices.data();
      vertexBufferData.SysMemPitch = 0;
      vertexBufferData.SysMemSlicePitch = 0;

      const CD3D11_BUFFER_DESC vertexBufferDesc(
         sizeof(VertexPositionTextureCoords) * static_cast<UINT>(quadVertices.size()),
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

void QuadRenderer::ReleaseDeviceDependentResources()
{
   _loadingComplete = false;
   _vertexShader.Reset();
   _inputLayout.Reset();
   _pixelShader.Reset();
   _modelConstantBuffer.Reset();
   _vertexBuffer.Reset();
}

void QuadRenderer::UpdatePosition(SpatialPointerPose^ pointerPose)
{
   if (pointerPose != nullptr)
   {
      _headForwardDirection = pointerPose->Head->ForwardDirection;
      _headUpDirection = pointerPose->Head->UpDirection;

      _quadPosition = pointerPose->Head->Position + _headForwardDirection * 4.0;
   }
}

void QuadRenderer::Update()
{
   const float3 quadNormal(0, 0, -1);
   float3 forwardDirection(_headForwardDirection.x, 0, _headForwardDirection.z);

   //Rotate quad to always face the view.
   XMVECTOR angle = XMVector3AngleBetweenVectors(XMLoadFloat3(&forwardDirection), XMLoadFloat3(&quadNormal));
   auto downDirection = cross(forwardDirection, quadNormal);

   if (dot(downDirection, _headUpDirection) > 0)
   {
      //Invert angle when facing the other direction.
      float3 tempAngle;
      XMStoreFloat3(&tempAngle, angle);
      tempAngle *= -1;
      angle = XMLoadFloat3(&tempAngle);
   }

   //Create transforms.
   const XMMATRIX rotation = XMMatrixRotationY(XMVectorGetY(angle));
   const XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&_quadPosition));

   //Store model transform.
   XMStoreFloat4x4(&_modelConstantBufferData.model, XMMatrixTranspose(rotation * translation));

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
      0
   );
}

void QuadRenderer::Render(const DepthTexture& depthTexture)
{
   if (!_loadingComplete)
   {
      return;
   }

   const auto context = _deviceResources->GetD3DDeviceContext();

   //Bind buffers.
   const UINT stride = sizeof(VertexPositionTextureCoords);
   const UINT offset = 0;
   context->IASetVertexBuffers(
      0,
      1,
      _vertexBuffer.GetAddressOf(),
      &stride,
      &offset
   );

   context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
   context->IASetInputLayout(_inputLayout.Get());

   //Bind shaders.
   context->VSSetShader(
      _vertexShader.Get(),
      nullptr,
      0
   );

   context->VSSetConstantBuffers(
      0,
      1,
      _modelConstantBuffer.GetAddressOf()
   );

   context->PSSetShader(
      _pixelShader.Get(),
      nullptr,
      0
   );

   context->PSSetSamplers(0, 1, _sampler.GetAddressOf());

   auto texture = depthTexture.GetTextureView();
   if (texture != nullptr)
   {
      context->PSSetShaderResources(0, 1, &texture);
   }

   //Draw.
   context->DrawInstanced(_vertexCount, 2, 0, 0);
}
