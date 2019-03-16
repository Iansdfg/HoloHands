#pragma once

namespace HoloHands
{
    class AxisRenderer
    {
    public:
        AxisRenderer(_In_ const Graphics::DeviceResourcesPtr& deviceResources, float alpha);

        void CreateDeviceDependentResources();

        void ReleaseDeviceDependentResources();

        void Update(const Windows::Foundation::Numerics::float4x4& transform);

        void Render();

        void SetPosition(
           Windows::Foundation::Numerics::float3 pos)
        {
           _position = pos;
        }

        Windows::Foundation::Numerics::float3 GetPosition()
        {
           return _position;
        }

    private:
        // Cached pointer to device resources.
        Graphics::DeviceResourcesPtr _deviceResources;

        std::unique_ptr<Rendering::SlateMaterial> _slateMaterial;

        Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _modelConstantBuffer;

        Rendering::SlateModelConstantBuffer _modelConstantBufferData;
        uint32 _indexCount = 0;

        bool _loadingComplete = false;
        int _vertexCount = 0;
        Windows::Foundation::Numerics::float3 _position = { 0.f, 0.f, 0.f };
        float _alpha = 1;
    };
}
