#include "pch.h"

#include "AxisRenderer.h"

namespace HoloHands
{
    // Loads vertex and pixel shaders from files and instantiates the cube geometry.
    AxisRenderer::AxisRenderer(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources, float alpha)
        : _deviceResources(deviceResources),
       _alpha(alpha)
    {
        CreateDeviceDependentResources();
    }

    void AxisRenderer::Update(const Windows::Foundation::Numerics::float4x4& transform)
    {
       const DirectX::XMMATRIX modelTranslation = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&_position));


        const DirectX::XMMATRIX axisTransform = DirectX::XMLoadFloat4x4(&transform);


        const DirectX::XMMATRIX modelTransform = XMMatrixMultiply(axisTransform, modelTranslation);

        XMStoreFloat4x4(&_modelConstantBufferData.model, DirectX::XMMatrixTranspose(modelTransform));

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

    void AxisRenderer::Render()
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

        // Apply the model constant buffer to the vertex shader.
        context->VSSetConstantBuffers(
            0,
            1,
            _modelConstantBuffer.GetAddressOf()
        );

        // Set pixel shader resources
        {
            ID3D11ShaderResourceView* shaderResourceViews[1] =
            {
                nullptr
            };

            context->PSSetShaderResources(
                0 /* StartSlot */,
                1 /* NumViews */,
                shaderResourceViews);
        }

        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

        // Draw the objects.
        context->DrawInstanced(
            _vertexCount,   // Index count per instance.
            2,              // Instance count.
            0,              // Start index location.
            0               // Start instance location.
        );
    }

    void AxisRenderer::CreateDeviceDependentResources()
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
           static const std::array<Rendering::VertexPositionColorTexture, 6> cubeVertices =
           {
              {
               { { 0, 0, 0 }, { _alpha, 0.0f, 0.0f }, { 0.0f, 0.0f } },
               { { 1, 0, 0 }, { _alpha, 0.0f, 0.0f }, { 0.0f, 0.0f } },
               { { 0, 0, 0 }, { 0.0f, _alpha, 0.0f }, { 0.0f, 0.0f } },
               { { 0, 1, 0 }, { 0.0f, _alpha, 0.0f }, { 0.0f, 0.0f } },
               { { 0, 0, 0 }, { 0.0f, 0.0f, _alpha }, { 0.0f, 0.0f } },
               { { 0, 0, 1 }, { 0.0f, 0.0f, _alpha }, { 0.0f, 0.0f } },
           }
           };

           _vertexCount = cubeVertices.size();

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
        }


        _loadingComplete = true;
    }

    void AxisRenderer::ReleaseDeviceDependentResources()
    {
        _loadingComplete = false;

        _modelConstantBuffer.Reset();
        _vertexBuffer.Reset();

        if (nullptr != _slateMaterial)
        {
            _slateMaterial->ReleaseDeviceDependentResources();
        }
    }
}
