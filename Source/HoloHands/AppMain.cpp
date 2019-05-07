#include "pch.h"

#include "AppMain.h"
#include "Utils/MathsUtils.h"
#include "Utils/IOUtils.h"

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
      //Add cubes to scene.
      _cubePositions.push_back({ 0.5f, 0.0f, 0.0f });
      _cubePositions.push_back({ 0.0f, 0.0f, 0.5f });
      _cubePositions.push_back({-0.5f, 0.0f, 0.0f });
   }

   void AppMain::OnHolographicSpaceChanged(
      Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
   {
      StartHoloLensMediaFrameSourceGroup();

      //Create renderers.
      _axisRenderer = std::make_unique<HoloHands::AxisRenderer>(_deviceResources);
      _cubeRenderer = std::make_unique<CubeRenderer>(_deviceResources, _cubeSize);
      _quadRenderer = std::make_unique<QuadRenderer>(_deviceResources);
      _crosshairRenderer = std::make_unique<CrosshairRenderer>(_deviceResources);
      _markerRenderer = std::make_unique<MarkerRenderer>(_deviceResources);

      //Create hand detector.
      _handDetector = std::make_unique<HoloHands::HandDetector>();
      _depthTexture = std::make_unique<DepthTexture>(_deviceResources);
      _handDetector->ShowDebugInfo(_showDebugInfo);
   }

   void AppMain::OnSpatialInput(SpatialInteractionSourceState^ pointerState)
   {
      bool isClosed = pointerState->IsPressed;

      _handDetector->SetIsClosed(isClosed);

      if (isClosed)
      {
         _selectedCubeIndex = NearestCube();
      }
      else
      {
         _selectedCubeIndex = -1;
      }

   }

   void AppMain::OnUpdate(
      Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
      const Graphics::StepTimer& stepTimer)
   {
      if (!_holoLensMediaFrameSourceGroupStarted)
      {
         return;
      }

      //Get image from depth sensor.
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
         _crosshairRenderer->SetColor(GetCrosshairColor());

         //Render debug information.
         auto cs = _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem;
         HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;
         SpatialPointerPose^ pose = SpatialPointerPose::TryGetAtTimestamp(cs, prediction->Timestamp);
         _quadRenderer->UpdatePose(pose);
         _quadRenderer->Update();

         _markerRenderer->UpdatePose(pose);

         _depthTexture->CopyFrom(_handDetector->GetDebugImage());
         _crosshairRenderer->SetPosition(_handPosition);
         _crosshairRenderer->Update();
      }
   }

   void AppMain::OnPreRender()
   {
      //Do nothing.
   }

   bool InsideFrustum(const float3& position)
   {
	   //TODO:
	   return false;
   }

   void AppMain::OnRender()
   {
      //Render cubes.
      for(auto& position : _cubePositions)
      {
         _cubeRenderer->SetPosition(position);
         _cubeRenderer->Update();
         _cubeRenderer->Render();

		 if (!InsideFrustum(position))
		 {
			 _markerRenderer->SetPosition(position);
			 _markerRenderer->Update();
			 _markerRenderer->Render();
		 }
      }

      if (_showDebugInfo)
      {
         //Render debug info.
         if (_handFound)
         {
            _crosshairRenderer->Render();
         }

         _quadRenderer->Render(*_depthTexture);
      }
   }

   void AppMain::OnDeviceLost()
   {
      _cubeRenderer->ReleaseDeviceDependentResources();
      _axisRenderer->ReleaseDeviceDependentResources();
      _quadRenderer->ReleaseDeviceDependentResources();
      _crosshairRenderer->ReleaseDeviceDependentResources();
      _markerRenderer->ReleaseDeviceDependentResources();

      _depthTexture->ReleaseDeviceDependentResources();

      _holoLensMediaFrameSourceGroup = nullptr;
      _holoLensMediaFrameSourceGroupStarted = false;
   }

   void AppMain::OnDeviceRestored()
   {
      _cubeRenderer->CreateDeviceDependentResources();
      _axisRenderer->CreateDeviceDependentResources();
      _quadRenderer->CreateDeviceDependentResources();
      _crosshairRenderer->CreateDeviceDependentResources();
      _markerRenderer->CreateDeviceDependentResources();

      _depthTexture->CreateDeviceDependentResources();

      StartHoloLensMediaFrameSourceGroup();
   }


   int AppMain::NearestCube()
   {
      //Only select cubes on closed pose.

      const float pickArea = _pickingTolerance + _cubeSize;
      for (int i = 0; i < static_cast<int>(_cubePositions.size()); i++)
      {
         if (length(_handPosition - _cubePositions[i]) < pickArea)
         {
            //Cube position is near the hand position.
            return i;
         }
      }

      return -1;
   }

   float3 AppMain::GetCrosshairColor()
   {
      if (_handDetector->GetIsClosed())
      {
         //Closed hand.
         if (_selectedCubeIndex == -1)
         {
            //Selected nothing.
            return float3(1.0f, 0.3f, 0.3f);
         }
         else
         {
            //Cube selected.
            return float3(0.3f, 1.f, 0.3f);
         }
      }
      else
      {
         //Open hand.
         if (NearestCube() != -1)
         {
            //Over cube.
            return float3(0.3f, 1.f, 0.3f);
         }
         else
         {
            //Over nothing.
            return float3(1.f, 1.f, 1.f);
         }
      }
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

      Eigen::Matrix4f camToOrigin = MathsUtils::Convert(viewToFrame * frame->FrameToOrigin);
	  frame->CameraProjectionTransform;
      //Convert from UV space to XY direction.
      Point uv(
         static_cast<float>(position2D.x),
         static_cast<float>(position2D.y));

      Point xy;
      frame->SensorStreamingCameraIntrinsics->MapImagePointToCameraUnitPlane(uv, &xy);

      //Add depth to direction.
      Eigen::Vector3f direction;
      direction[0] = -xy.X;
      direction[1] = -xy.Y;
      direction[2] = -1.0f;

      const float depthScale = 0.001f;
      direction.normalize();
      direction *= depth * depthScale;

      //Transform into world space.
      Eigen::Vector4f worldPosition =
         camToOrigin.transpose() *
         Eigen::Vector4f(direction.x(), direction.y(), direction.z(), 1);

      handPosition = float3(
         worldPosition.x(),
         worldPosition.y(),
         worldPosition.z());

      return true;
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

      //Enable depth sensor.
      _holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::ShortThrowToFDepth);

      concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then([&]()
      {
         _holoLensMediaFrameSourceGroupStarted = true;
      });
   }
}
