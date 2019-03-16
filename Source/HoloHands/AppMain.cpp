#include "pch.h"

#include "AppMain.h"
#include "Utils/MathsUtils.h"

using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Input::Spatial;

namespace HoloHands
{
   AppMain::AppMain(
      const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
      :
      Holographic::AppMainBase(deviceResources),
      _selectedHoloLensMediaFrameSourceGroupType(HoloLensForCV::MediaFrameSourceGroupType::HoloLensResearchModeSensors),
      _holoLensMediaFrameSourceGroupStarted(false),
      _showDebugInfo(true)
   {
   }

   void AppMain::OnHolographicSpaceChanged(
      Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
   {
      StartHoloLensMediaFrameSourceGroup();

      _handDetector = std::make_unique<HoloHands::HandDetector>();
      _axisRenderer = std::make_unique<HoloHands::AxisRenderer>(_deviceResources);
      _cubeRenderer = std::make_unique<CubeRenderer>(_deviceResources);
      _quadRenderer = std::make_unique<QuadRenderer>(_deviceResources);
      _depthTexture = std::make_unique<DepthTexture>(_deviceResources);

      _handDetector->ShowDebugInfo(_showDebugInfo);
   }

   void AppMain::OnSpatialInput(Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState)
   {
      //Do nothing.
   }

   void AppMain::OnUpdate(
      Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
      const Graphics::StepTimer& stepTimer)
   {
      if (!_holoLensMediaFrameSourceGroupStarted)
      {
         return;
      }

      HoloLensForCV::SensorFrame^ latestFrame =
         _holoLensMediaFrameSourceGroup->GetLatestSensorFrame(HoloLensForCV::SensorType::ShortThrowToFDepth);

      if (latestFrame == nullptr ||
         _latestSelectedCameraTimestamp.UniversalTime == latestFrame->Timestamp.UniversalTime)
      {
         return;
      }

      _latestSelectedCameraTimestamp = latestFrame->Timestamp;

      cv::Mat image;

      rmcv::WrapHoloLensSensorFrameWithCvMat(
         latestFrame,
         image);

      bool handFound = _handDetector->Process(image);

      cv::Point handPosition2D = _handDetector->GetHandPosition();
      float handDepth = _handDetector->GetHandDepth();

      if (handDepth < 200 || handDepth > 1000)
      {
         handFound = false;
      }

      float4x4 camToRef;

      invert(latestFrame->CameraViewTransform, &camToRef);

      float4x4 camToOrigin = camToRef * latestFrame->FrameToOrigin;
      Eigen::Vector3f camPinhole(camToOrigin.m41, camToOrigin.m42, camToOrigin.m43);

      Windows::Foundation::Point uv;

      uv.X = static_cast<float>(handPosition2D.x);
      uv.Y = static_cast<float>(handPosition2D.y);

      Windows::Foundation::Point xy;

      latestFrame->SensorStreamingCameraIntrinsics->MapImagePointToCameraUnitPlane(uv, &xy);

      Eigen::Vector3f dirCam;

      dirCam[0] = -xy.X;
      dirCam[1] = -xy.Y;
      dirCam[2] = -1.0f;

      dirCam.normalize();
      dirCam *= handDepth * 0.001f;

      Eigen::Matrix3f finalTransform = MathsUtils::Convert(camToOrigin).transpose();
      Eigen::Vector3f dir = finalTransform * dirCam;

      Eigen::Vector3f worldPosition = camPinhole + dir;

      float3 p(
         worldPosition.x(),
         worldPosition.y(),
         worldPosition.z());

      if (handFound)
      {
         _cubeRenderer->SetPosition(p);

         _cubeRenderer->Update();
      }

      if (_showDebugInfo)
      {
         auto cs = _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem;
         HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;
         SpatialPointerPose^ pose = SpatialPointerPose::TryGetAtTimestamp(cs, prediction->Timestamp);
         _quadRenderer->UpdatePosition(pose);
         _quadRenderer->Update();

         _depthTexture->CopyFrom(_handDetector->GetDebugImage());

         _axisRenderer->SetPosition(p);
         _axisRenderer->SetTransform(float4x4::identity());
         _axisRenderer->Update();
      }
   }

   void AppMain::OnPreRender()
   {
      //Do nothing.
   }

   void AppMain::OnRender()
   {
      _cubeRenderer->Render();

      if (_showDebugInfo)
      {
         _axisRenderer->Render();

         _quadRenderer->Render(*_depthTexture);
      }
   }

   void AppMain::OnDeviceLost()
   {
      _cubeRenderer->ReleaseDeviceDependentResources();
      _axisRenderer->ReleaseDeviceDependentResources();
      _quadRenderer->ReleaseDeviceDependentResources();
      _depthTexture->ReleaseDeviceDependentResources();

      _holoLensMediaFrameSourceGroup = nullptr;
      _holoLensMediaFrameSourceGroupStarted = false;
   }

   void AppMain::OnDeviceRestored()
   {
      _cubeRenderer->CreateDeviceDependentResources();
      _axisRenderer->CreateDeviceDependentResources();
      _quadRenderer->CreateDeviceDependentResources();
      _depthTexture->CreateDeviceDependentResources();

      StartHoloLensMediaFrameSourceGroup();
   }

   void AppMain::StartHoloLensMediaFrameSourceGroup()
   {
      _sensorFrameStreamer =
         ref new HoloLensForCV::SensorFrameStreamer();

      _holoLensMediaFrameSourceGroup =
         ref new HoloLensForCV::MediaFrameSourceGroup(
            _selectedHoloLensMediaFrameSourceGroupType,
            _spatialPerception,
            _sensorFrameStreamer);

      _holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::ShortThrowToFDepth);

      concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then(
         [&]()
      {
         _holoLensMediaFrameSourceGroupStarted = true;
      });
   }
}
