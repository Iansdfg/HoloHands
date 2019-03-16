//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

namespace HoloHands
{
    class CubeRenderer
    {
    public:
       CubeRenderer(const Graphics::DeviceResourcesPtr& deviceResources);

        void CreateDeviceDependentResources();

        void ReleaseDeviceDependentResources();

        void Update(const Graphics::StepTimer& timer);

        void Render();

        void PositionHologram(Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose);

        void SetPosition(Windows::Foundation::Numerics::float3 pos)
        {
            _position = pos;
        }

        Windows::Foundation::Numerics::float3 GetPosition()
        {
            return _position;
        }

    private:
        Graphics::DeviceResourcesPtr _deviceResources;

        std::unique_ptr<Rendering::SlateMaterial> _slateMaterial;

        Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _modelConstantBuffer;

        Rendering::SlateModelConstantBuffer _modelConstantBufferData;
        uint32 _indexCount = 0;

        bool _loadingComplete = false;

        Windows::Foundation::Numerics::float3 _position = { 0.f, 0.f, 0.f };
        float _rotationInRadians;
    };
}
