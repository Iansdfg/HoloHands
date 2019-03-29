#pragma once

#include "CV/HandDetector.h"
#include "Rendering/AxisRenderer.h"
#include "Rendering/CubeRenderer.h"
#include "Rendering/QuadRenderer.h"
#include "Rendering/CrosshairRenderer.h"
#include "Rendering/DepthTexture.h"

namespace HoloHands
{
   class AppMain : public Holographic::AppMainBase
   {
   public:
      AppMain(const std::shared_ptr<Graphics::DeviceResources>& deviceResources);

      virtual void OnDeviceLost() override;

      virtual void OnDeviceRestored() override;

      virtual void OnHolographicSpaceChanged(
         Windows::Graphics::Holographic::HolographicSpace^ holographicSpace) override;

      virtual void OnSpatialInput(
         Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState) override;

      virtual void OnUpdate(
         Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
         const Graphics::StepTimer& stepTimer) override;

      virtual void OnPreRender() override;

      virtual void OnRender() override;

   private:
      // Get a 3D hand position in world space from a given frame.
      // Returns false if hand is not found and the position as an out parameter.
      bool GetHandPositionFromFrame(
         HoloLensForCV::SensorFrame^ frame,
         Windows::Foundation::Numerics::float3& handPosition);

      // Select a cube at the current hand position.
      // Returns -1 if no cube is found at the position.
      int SelectCube(bool handIsClosed);

      void StartHoloLensMediaFrameSourceGroup();

      std::unique_ptr<CubeRenderer> _cubeRenderer;
      std::unique_ptr<AxisRenderer> _axisRenderer;
      std::unique_ptr<QuadRenderer> _quadRenderer;
      std::unique_ptr<CrosshairRenderer> _crosshairRenderer;

      std::unique_ptr<HandDetector> _handDetector;
      std::unique_ptr<DepthTexture> _depthTexture;

      Windows::Foundation::Numerics::float3 _handPosition;
      std::vector<Windows::Foundation::Numerics::float3> _cubePositions;
      float _cubeSize;
      float _pickingTolerance;
      int _selectedCubeIndex;
      bool _handFound;
      bool _showDebugInfo;

      HoloLensForCV::MediaFrameSourceGroupType _selectedHoloLensMediaFrameSourceGroupType;
      HoloLensForCV::MediaFrameSourceGroup^ _holoLensMediaFrameSourceGroup;
      bool _holoLensMediaFrameSourceGroupStarted;
      HoloLensForCV::SensorFrameStreamer^ _sensorFrameStreamer;
      Windows::Foundation::DateTime _latestSelectedCameraTimestamp;
   };
}
