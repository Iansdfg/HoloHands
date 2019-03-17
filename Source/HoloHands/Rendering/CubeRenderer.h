#pragma once

namespace HoloHands
{
   class CubeRenderer
   {
   public:
      CubeRenderer(
         const std::shared_ptr<Graphics::DeviceResources>& deviceResources,
         float size);

      void CreateDeviceDependentResources();
      void ReleaseDeviceDependentResources();

      void Update();
      void Render();

      void SetPosition(const Windows::Foundation::Numerics::float3& pos);
      Windows::Foundation::Numerics::float3 GetPosition();

   private:
      Graphics::DeviceResourcesPtr _deviceResources;
      std::unique_ptr<Rendering::SlateMaterial> _slateMaterial;

      Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
      Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer;
      Microsoft::WRL::ComPtr<ID3D11Buffer> _modelConstantBuffer;

      Rendering::SlateModelConstantBuffer _modelConstantBufferData;
      uint32 _indexCount;
      bool _loadingComplete;
      Windows::Foundation::Numerics::float3 _position;
      float _size;
   };
}
