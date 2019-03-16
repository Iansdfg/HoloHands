#pragma once

namespace HoloHands
{
    class AxisRenderer
    {
    public:
        AxisRenderer(const std::shared_ptr<Graphics::DeviceResources>& deviceResources);
        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();

        void Update();
        void Render();

        void SetPosition(const Windows::Foundation::Numerics::float3& position);
        void SetTransform(const Windows::Foundation::Numerics::float4x4& transform);

    private:
        Graphics::DeviceResourcesPtr _deviceResources;
        std::unique_ptr<Rendering::SlateMaterial> _slateMaterial;

        Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _modelConstantBuffer;
        Rendering::SlateModelConstantBuffer _modelConstantBufferData;

        int _vertexCount = 0;
        bool _loadingComplete = false;
        Windows::Foundation::Numerics::float3 _position;
        Windows::Foundation::Numerics::float4x4 _transform;
    };
}
