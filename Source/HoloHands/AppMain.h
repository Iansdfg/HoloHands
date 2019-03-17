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

#include "CV/HandDetector.h"
#include "Rendering/AxisRenderer.h"
#include "Rendering/CubeRenderer.h"
#include "Rendering/QuadRenderer.h"
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
      bool GetHandPositionFromFrame(HoloLensForCV::SensorFrame^ frame, Windows::Foundation::Numerics::float3& handPosition);
      void StartHoloLensMediaFrameSourceGroup();
      int SelectCube(bool handIsClosed);

      std::unique_ptr<CubeRenderer> _cubeRenderer;
      std::unique_ptr<AxisRenderer> _axisRenderer;
      std::unique_ptr<QuadRenderer> _quadRenderer;
      std::unique_ptr<HandDetector> _handDetector;
      std::unique_ptr<DepthTexture> _depthTexture;

      Windows::Foundation::Numerics::float3 _handPosition;
      std::vector<Windows::Foundation::Numerics::float3> _cubePositions;
      float _cubeSize;
      float _pickingTolerance;
      int _selectedCubeIndex;
      bool _handFound;

      HoloLensForCV::MediaFrameSourceGroupType _selectedHoloLensMediaFrameSourceGroupType;
      HoloLensForCV::MediaFrameSourceGroup^ _holoLensMediaFrameSourceGroup;
      bool _holoLensMediaFrameSourceGroupStarted;

      HoloLensForCV::SensorFrameStreamer^ _sensorFrameStreamer;

      Windows::Foundation::DateTime _latestSelectedCameraTimestamp;

      bool _showDebugInfo;
   };
}
