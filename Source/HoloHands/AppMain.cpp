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
      _showDebugInfo(true),
      _handFound(false),
      _cubeSize(0.01f),
      _pickingTolerance(0.03f),
      _selectedCubeIndex(-1)
   {
      _cubePositions.push_back({ 0.5, 0, 0 });
      _cubePositions.push_back({ 0, 0, 0.5 });
      _cubePositions.push_back({ -0.5, 0, 0 });
   }

   void AppMain::OnHolographicSpaceChanged(
      Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
   {
      StartHoloLensMediaFrameSourceGroup();

      _handDetector = std::make_unique<HoloHands::HandDetector>();
      _axisRenderer = std::make_unique<HoloHands::AxisRenderer>(_deviceResources);
      _cubeRenderer = std::make_unique<CubeRenderer>(_deviceResources, _cubeSize);
      _quadRenderer = std::make_unique<QuadRenderer>(_deviceResources);
      _depthTexture = std::make_unique<DepthTexture>(_deviceResources);

      _handDetector->ShowDebugInfo(_showDebugInfo);
   }

   void AppMain::OnSpatialInput(Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState)
   {
      _handDetector->SetIsClosed(pointerState->IsPressed);

      _selectedCubeIndex = SelectCube(pointerState->IsPressed);
   }

   int AppMain::SelectCube(bool handIsClosed)
   {
      if (handIsClosed)
      {
         const float pickArea = _pickingTolerance + _cubeSize;
         for (int i = 0; i < static_cast<int>(_cubePositions.size()); i++)
         {
            if (length(_handPosition - _cubePositions[i]) < pickArea)
            {
               return i;
            }
         }
      }

      return -1;
   }

   bool AppMain::GetHandPositionFromFrame(HoloLensForCV::SensorFrame^ frame, float3& handPosition)
   {
      cv::Mat image;
      rmcv::WrapHoloLensSensorFrameWithCvMat(frame, image);

      //Detect 2D hand position and depth from OpenCV Mat.
      _handFound = _handDetector->Process(image);
      float depth = _handDetector->GetHandDepth();
      cv::Point position2D = _handDetector->GetHandPosition2D();

      if (_handFound == false || depth < 200 || depth > 1000)
      {
         //Invalid hand position detected.
         return false;
      }

      //Calculate transforms.
      float4x4 viewToFrame;
      invert(frame->CameraViewTransform, &viewToFrame);

      float4x4 camToOrigin = viewToFrame * frame->FrameToOrigin;
      Eigen::Vector3f camToOriginTranslation(camToOrigin.m41, camToOrigin.m42, camToOrigin.m43);
      Eigen::Matrix3f camToOriginRotation = MathsUtils::Convert(camToOrigin);

      //Convert from UV space to XY direction.
      Point uv(
         static_cast<float>(position2D.x),
         static_cast<float>(position2D.y));

      Point xy;
      frame->SensorStreamingCameraIntrinsics->MapImagePointToCameraUnitPlane(uv, &xy);

      //Add depth to direction.
      Eigen::Vector3f depthDirection;
      depthDirection[0] = -xy.X;
      depthDirection[1] = -xy.Y;
      depthDirection[2] = -1.0f;

      depthDirection.normalize();
      depthDirection *= depth * 0.001f;

      //Transform into world space.
      Eigen::Vector3f worldDirection = camToOriginRotation.transpose() * depthDirection;
      Eigen::Vector3f worldPosition = camToOriginTranslation + worldDirection;

      handPosition = float3(
         worldPosition.x(),
         worldPosition.y(),
         worldPosition.z());

      return true;
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


      if (GetHandPositionFromFrame(latestFrame, _handPosition))
      {
         //Move cube to hand position.
         if (_selectedCubeIndex != -1)
         {
            _cubePositions[_selectedCubeIndex] = _handPosition;
         }
      }

      if (_showDebugInfo)
      {
         auto cs = _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem;
         HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;
         SpatialPointerPose^ pose = SpatialPointerPose::TryGetAtTimestamp(cs, prediction->Timestamp);
         _quadRenderer->UpdatePosition(pose);
         _quadRenderer->Update();

         _depthTexture->CopyFrom(_handDetector->GetDebugImage());

         _axisRenderer->SetPosition(_handPosition);
         _axisRenderer->Update();

         if (_selectedCubeIndex != -1)
         {
            //TODO: Visual feedback on selection.
         }
      }
   }

   void AppMain::OnPreRender()
   {
      //Do nothing.
   }

   void AppMain::OnRender()
   {
      for (int i = 0; i < _cubePositions.size(); i++)
      {
         _cubeRenderer->SetPosition(_cubePositions[i]);
         _cubeRenderer->Update();
         _cubeRenderer->Render();
      }

      if (_showDebugInfo)
      {
         if (_handFound)
         {
            _axisRenderer->Render();
         }

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
