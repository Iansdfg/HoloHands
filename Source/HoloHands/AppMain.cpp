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
   }

   void AppMain::OnSpatialInput(Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState)
   {
      //Do nothing.
   }

   void AppMain::OnUpdate(
      Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
      const Graphics::StepTimer& stepTimer)
   {
      _cubeRenderer->Update(stepTimer);

      if (!_holoLensMediaFrameSourceGroupStarted)
      {
         return;
      }

      HoloLensForCV::SensorFrame^ latestFrame =
         _holoLensMediaFrameSourceGroup->GetLatestSensorFrame(HoloLensForCV::SensorType::ShortThrowToFDepth);

      if (nullptr == latestFrame)
      {
         return;
      }

      if (_latestSelectedCameraTimestamp.UniversalTime == latestFrame->Timestamp.UniversalTime)
      {
         return;
      }

      _latestSelectedCameraTimestamp = latestFrame->Timestamp;

      cv::Mat wrappedImage;

      rmcv::WrapHoloLensSensorFrameWithCvMat(
         latestFrame,
         wrappedImage);

      HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

      _handDetector->Process(wrappedImage);
      auto imagePos = _handDetector->GetLeftHandPosition();

      auto depth = wrappedImage.at<unsigned short>(_handDetector->GetLeftHandCenter());
      float depthF = static_cast<float>(depth);

      if (depthF < 200 || depthF > 1000)
      {
         return;
      }

      Windows::Foundation::Numerics::float4x4 camToRef;

      Windows::Foundation::Numerics::invert(latestFrame->CameraViewTransform, &camToRef);

      Windows::Foundation::Numerics::float4x4 camToOrigin = camToRef * latestFrame->FrameToOrigin;
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
      dirCam *= depthF * 0.001f;

      Eigen::Matrix3f finalTransform = camToOriginR.transpose();
      Eigen::Vector3f dir = finalTransform * dirCam;

      Eigen::Vector3f worldPosition = camPinhole + dir;

      Windows::Foundation::Numerics::float3 p(
         worldPosition.x(),
         worldPosition.y(),
         worldPosition.z());

      _cubeRenderer->SetPosition(p);

      _axisRenderer->SetPosition(p);
      _axisRenderer->SetTransform(Windows::Foundation::Numerics::float4x4::identity());
      _axisRenderer->Update();
   }

   void AppMain::OnPreRender()
   {
      //Do nothing.
   }

   void AppMain::OnRender()
   {
      _cubeRenderer->Render();
      _axisRenderer->Render();
   }

   void AppMain::OnDeviceLost()
   {
      _cubeRenderer->ReleaseDeviceDependentResources();
      _axisRenderer->ReleaseDeviceDependentResources();

      _holoLensMediaFrameSourceGroup = nullptr;
      _holoLensMediaFrameSourceGroupStarted = false;
   }

   void AppMain::OnDeviceRestored()
   {
      _cubeRenderer->CreateDeviceDependentResources();
      _axisRenderer->CreateDeviceDependentResources();

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
