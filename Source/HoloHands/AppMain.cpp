#include "pch.h"

#include "AppMain.h"

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
      _debugView(true)
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

      _handDetector->Process(image);
      auto imagePos = _handDetector->GetLeftHandPosition();

      float depth = static_cast<float>(image.at<unsigned short>(_handDetector->GetLeftHandCenter()));

      //if (depth < 200 || depth > 1000)
      //{
      //   //Invalid depth.
      //   return;
      //}

      float4x4 camToRef;

      invert(latestFrame->CameraViewTransform, &camToRef);

      float4x4 camToOrigin = camToRef * latestFrame->FrameToOrigin;
      Eigen::Vector3f camPinhole(camToOrigin.m41, camToOrigin.m42, camToOrigin.m43);

      Eigen::Matrix3f camToOriginR;

      camToOriginR(0, 0) = camToOrigin.m11;
      camToOriginR(0, 1) = camToOrigin.m12;
      camToOriginR(0, 2) = camToOrigin.m13;
      camToOriginR(1, 0) = camToOrigin.m21;
      camToOriginR(1, 1) = camToOrigin.m22;
      camToOriginR(1, 2) = camToOrigin.m23;
      camToOriginR(2, 0) = camToOrigin.m31;
      camToOriginR(2, 1) = camToOrigin.m32;
      camToOriginR(2, 2) = camToOrigin.m33;

      Windows::Foundation::Point uv;

      uv.X = static_cast<float>(imagePos.x);
      uv.Y = static_cast<float>(imagePos.y);

      Windows::Foundation::Point xy;

      latestFrame->SensorStreamingCameraIntrinsics->MapImagePointToCameraUnitPlane(uv, &xy);

      Eigen::Vector3f dirCam;

      dirCam[0] = -xy.X;
      dirCam[1] = -xy.Y;
      dirCam[2] = -1.0f;

      dirCam.normalize();
      dirCam *= depth * 0.001f;

      Eigen::Matrix3f finalTransform = camToOriginR.transpose();
      Eigen::Vector3f dir = finalTransform * dirCam;

      Eigen::Vector3f worldPosition = camPinhole + dir;

      float3 p(
         worldPosition.x(),
         worldPosition.y(),
         worldPosition.z());

      _cubeRenderer->SetPosition(p);

      _cubeRenderer->Update();

      if (_debugView)
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

      if (_debugView)
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

      _sensorFrameStreamer->EnableAll();

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
