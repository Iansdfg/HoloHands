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
      : Holographic::AppMainBase(deviceResources)
      , _selectedHoloLensMediaFrameSourceGroupType(
         HoloLensForCV::MediaFrameSourceGroupType::HoloLensResearchModeSensors)
      , _holoLensMediaFrameSourceGroupStarted(false)
      , _undistortMapsInitialized(false)
   {
      _handDetector = std::make_unique<HoloHands::HandDetector>();
   }

   void AppMain::OnHolographicSpaceChanged(
      Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
   {
      //
      // Initialize the HoloLens media frame readers
      //
      StartHoloLensMediaFrameSourceGroup();
   }

   void AppMain::OnSpatialInput(
      _In_ Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState)
   {
      _paused = pointerState->IsPressed;
      //Windows::Perception::Spatial::SpatialCoordinateSystem^ currentCoordinateSystem =
      //   _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem;

      //_renderer->PositionHologram(pointerState->TryGetPointerPose(currentCoordinateSystem));
   }

   Windows::Foundation::Numerics::float4x4 Convert(const Eigen::Matrix3f& mat)
   {
      auto m = Windows::Foundation::Numerics::float4x4::identity();

      m.m11 = mat(0, 0);
      m.m12 = mat(0, 1);
      m.m13 = mat(0, 2);
      m.m21 = mat(1, 0);
      m.m22 = mat(1, 1);
      m.m23 = mat(1, 2);
      m.m31 = mat(2, 0);
      m.m32 = mat(2, 1);
      m.m33 = mat(2, 2);

      return m;
   }

   const Eigen::Matrix3f Convert(const Windows::Foundation::Numerics::float4x4& mat)
   {
      Eigen::Matrix3f m = Eigen::Matrix3f::Identity();

      m(0, 0) = mat.m11; 
      m(0, 1) = mat.m12; 
      m(0, 2) = mat.m13; 
      m(1, 0) = mat.m21; 
      m(1, 1) = mat.m22; 
      m(1, 2) = mat.m23; 
      m(2, 0) = mat.m31; 
      m(2, 1) = mat.m32; 
      m(2, 2) = mat.m33; 

      return m;
   }

   void AppMain::OnUpdate(
      _In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
      _In_ const Graphics::StepTimer& stepTimer)
   {
      UNREFERENCED_PARAMETER(holographicFrame);

      dbg::TimerGuard timerGuard(
         L"AppMain::OnUpdate",
         30.0 /* minimum_time_elapsed_in_milliseconds */);


      if (!_renderer)
      {
         _renderer = std::make_shared<CubeRenderer>(_deviceResources);
      }
      _renderer->Update(stepTimer);

      if (!_worldCSRenderer)
      {
         _worldCSRenderer = std::make_unique<HoloHands::AxisRenderer>(_deviceResources, 1.f);
         _debugCSRenderer = std::make_unique<HoloHands::AxisRenderer>(_deviceResources, 1.f);
      }

      //
      // Process sensor data received through the HoloLensForCV component.
      //
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

      //if (!_undistortMapsInitialized)
      //{
      //   Windows::Media::Devices::Core::CameraIntrinsics^ cameraIntrinsics =
      //      latestFrame->CoreCameraIntrinsics;


      //   if (nullptr != cameraIntrinsics)
      //   {
      //      cv::Mat cameraMatrix(3, 3, CV_64FC1);

      //      cv::setIdentity(cameraMatrix);

      //      cameraMatrix.at<double>(0, 0) = cameraIntrinsics->FocalLength.x;
      //      cameraMatrix.at<double>(1, 1) = cameraIntrinsics->FocalLength.y;
      //      cameraMatrix.at<double>(0, 2) = cameraIntrinsics->PrincipalPoint.x;
      //      cameraMatrix.at<double>(1, 2) = cameraIntrinsics->PrincipalPoint.y;

      //      cv::Mat distCoeffs(5, 1, CV_64FC1);

      //      distCoeffs.at<double>(0, 0) = cameraIntrinsics->RadialDistortion.x;
      //      distCoeffs.at<double>(1, 0) = cameraIntrinsics->RadialDistortion.y;
      //      distCoeffs.at<double>(2, 0) = cameraIntrinsics->TangentialDistortion.x;
      //      distCoeffs.at<double>(3, 0) = cameraIntrinsics->TangentialDistortion.y;
      //      distCoeffs.at<double>(4, 0) = cameraIntrinsics->RadialDistortion.z;

      //      cv::initUndistortRectifyMap(
      //         cameraMatrix,
      //         distCoeffs,
      //         cv::Mat_<double>::eye(3, 3) /* R */,
      //         cameraMatrix,
      //         cv::Size(wrappedImage.cols, wrappedImage.rows),
      //         CV_32FC1 /* type */,
      //         _undistortMap1,
      //         _undistortMap2);

      //      _undistortMapsInitialized = true;
      //   }
      //}

      //if (_undistortMapsInitialized)
      //{
      //   cv::remap(
      //      wrappedImage,
      //      _undistortedPVCameraImage,
      //      _undistortMap1,
      //      _undistortMap2,
      //      cv::INTER_LINEAR);

      //   //cv::resize(
      //   //    _undistortedPVCameraImage,
      //   //    _resizedPVCameraImage,
      //   //    cv::Size(),
      //   //    0.5 /* fx */,
      //   //    0.5 /* fy */,
      //   //    cv::INTER_AREA);
      //}
      //else
      //{
      //   //cv::resize(
      //   //    wrappedImage,
      //   //    _resizedPVCameraImage,
      //   //    cv::Size(),
      //   //    0.5 /* fx */,
      //   //    0.5 /* fy */,
      //   //    cv::INTER_AREA);
      //}




      //if (_paused)
      //{
      //   return;
      //}

      auto cs = _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem;
      HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

      SpatialPointerPose^ pose = SpatialPointerPose::TryGetAtTimestamp(cs, prediction->Timestamp);
      //if (pose)
      //{
      //   auto headPos = pose->Head->ForwardDirection;

      //   OutputDebugStringA("head:\n");
      //   OutputDebugStringA(std::to_string(headPos.x).c_str());
      //   OutputDebugStringA(" ");
      //   OutputDebugStringA(std::to_string(headPos.y).c_str());
      //   OutputDebugStringA(" ");
      //   OutputDebugStringA(std::to_string(headPos.z).c_str());
      //   OutputDebugStringA("\n");
      //}

      _handDetector->Process(wrappedImage);
      auto imagePos = _handDetector->GetLeftHandPosition();

     // int width = latestFrame->SensorStreamingCameraIntrinsics->ImageWidth;
     // int height = latestFrame->SensorStreamingCameraIntrinsics->ImageHeight;
     //cv::Point imagePos(width / 2.0, height / 2.0); //TODO: using center.






      auto depth = wrappedImage.at<unsigned short>(_handDetector->GetLeftHandCenter());
      float depthF = static_cast<float>(depth);

      if (depthF < 200 || depthF > 1000)
      {
         return;
      }









      /* //Implementation 1
      Windows::Foundation::Numerics::float4x4 camToRef;
      Windows::Foundation::Numerics::invert(latestFrame->CameraViewTransform, &camToRef);

      auto camToOrigin = camToRef * latestFrame->FrameToOrigin;

      Eigen::Matrix4f camToOriginR;

      camToOriginR(0, 0) = camToOrigin.m11;
      camToOriginR(0, 1) = camToOrigin.m12;
      camToOriginR(0, 2) = camToOrigin.m13;
      camToOriginR(0, 3) = camToOrigin.m14;
      camToOriginR(1, 0) = camToOrigin.m21;
      camToOriginR(1, 1) = camToOrigin.m22;
      camToOriginR(1, 2) = camToOrigin.m23;
      camToOriginR(1, 3) = camToOrigin.m24;
      camToOriginR(2, 0) = camToOrigin.m31;
      camToOriginR(2, 1) = camToOrigin.m32;
      camToOriginR(2, 2) = camToOrigin.m33;
      camToOriginR(2, 3) = camToOrigin.m34;
      camToOriginR(3, 0) = camToOrigin.m41;
      camToOriginR(3, 1) = camToOrigin.m42;
      camToOriginR(3, 2) = camToOrigin.m43;
      camToOriginR(3, 3) = camToOrigin.m44;


      Windows::Foundation::Point uv;
      uv.X = static_cast<float>(imagePos.x);
      uv.Y = static_cast<float>(imagePos.y);

      Windows::Foundation::Point xy;
      latestFrame->SensorStreamingCameraIntrinsics->MapImagePointToCameraUnitPlane(uv, &xy);

      HoloLensForCV::SensorType type = latestFrame->FrameType;

      Eigen::Vector3f position;
      position[0] = xy.X;
      position[1] = xy.Y;
      position[2] = 1.0f;

      position.normalize();

      position *= depthF * 0.001;

      Eigen::Vector4f worldPosition = camToOriginR.transpose() * Eigen::Vector4f(position.x(), position.y(), position.z(), 1.f);

      */



      //Implementation 2
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


      //dirCam[0] = 0;
      //dirCam[1] = 0.5;
      //dirCam[2] = -1.0f;
      dirCam.normalize();
      dirCam *= depthF * 0.001;
      //dirCam *= 2.0;

      Eigen::Matrix3f finalTransform = camToOriginR.transpose();

      Eigen::Vector3f dir = finalTransform * dirCam;
      //dir = camToOriginR * dir;
      //OutputDebugStringA("world:\n");
      //OutputDebugStringA(std::to_string(dir.x()).c_str());
      //OutputDebugStringA(" ");
      //OutputDebugStringA(std::to_string(dir.y()).c_str());
      //OutputDebugStringA(" ");
      //OutputDebugStringA(std::to_string(dir.z()).c_str());
      //OutputDebugStringA("\n");


      //TODO: head forward direction should be approximately the same as dirCam, why is it not?


      //Eigen::Vector3f dir(pose->Head->ForwardDirection.x, pose->Head->ForwardDirection.y, pose->Head->ForwardDirection.z);
      //dir *= 1.0;// depthF * 0.001;

      Eigen::Vector3f worldPosition = camPinhole + dir;









      Windows::Foundation::Numerics::float3 p(
         worldPosition.x(),
         worldPosition.y(),
         worldPosition.z());

      _renderer->SetPosition(p);



      _worldCSRenderer->SetPosition(pose->Head->ForwardDirection);
      _debugCSRenderer->SetPosition(p);
      
      auto mat = Windows::Foundation::Numerics::float4x4::identity();
      _worldCSRenderer->Update(mat);
      _debugCSRenderer->Update(mat);

      //OutputDebugStringA("world:\n");
      //OutputDebugStringA(std::to_string(p.x).c_str());
      //OutputDebugStringA(" ");
      //OutputDebugStringA(std::to_string(p.y).c_str());
      //OutputDebugStringA(" ");
      //OutputDebugStringA(std::to_string(p.z).c_str());
      //OutputDebugStringA("\n");



      //auto type = wrappedImage.type();
      //auto channel = wrappedImage.channels();

      //wrappedImage.convertTo(wrappedImage, CV_8UC1);

      //cv::Mat rgbImage(450, 488, CV_8UC4);
      //cv::cvtColor(wrappedImage, rgbImage, cv::COLOR_GRAY2BGRA);



      //Mat scaled = original / MAX_IMAGE_DEPTH * 255.0; //scale to within 8bit range.
      //scaled.convertTo(scaled, CV_8UC1); //make correct format of opencv.

       //cv::medianBlur(
       //   rgbImage,
       //    _blurredPVCameraImage,
       //    3 /* ksize */);

       //cv::Canny(
       //    _blurredPVCameraImage,
       //    _cannyPVCameraImage,
       //    50.0,
       //    200.0);

       //for (int32_t y = 0; y < _blurredPVCameraImage.rows; ++y)
       //{
       //    for (int32_t x = 0; x < _blurredPVCameraImage.cols; ++x)
       //    {
       //        if (_cannyPVCameraImage.at<uint8_t>(y, x) > 64)
       //        {
       //            *(_blurredPVCameraImage.ptr<uint32_t>(y, x)) = 0xFFFF00FF;
       //        }
       //    }
       //}

      //OpenCVHelpers::CreateOrUpdateTexture2D(
      //   _deviceResources,
      //   rgbImage,
      //   _currentVisualizationTexture);


   }

   void AppMain::OnPreRender()
   {
   }

   // Renders the current frame to each holographic camera, according to the
   // current application and spatial positioning state.
   void AppMain::OnRender()
   {
      _renderer->Render();
      //_worldCSRenderer->Render();
      _debugCSRenderer->Render();
   }

   // Notifies classes that use Direct3D device resources that the device resources
   // need to be released before this method returns.
   void AppMain::OnDeviceLost()
   {

         _renderer->ReleaseDeviceDependentResources();
         _worldCSRenderer->ReleaseDeviceDependentResources();
         _debugCSRenderer->ReleaseDeviceDependentResources();

      _holoLensMediaFrameSourceGroup = nullptr;
      _holoLensMediaFrameSourceGroupStarted = false;

      for (auto& v : _visualizationTextureList)
      {
         v.reset();
      }
      _currentVisualizationTexture.reset();
   }

   // Notifies classes that use Direct3D device resources that the device resources
   // may now be recreated.
   void AppMain::OnDeviceRestored()
   {
      _renderer->CreateDeviceDependentResources();
      _worldCSRenderer->CreateDeviceDependentResources();
      _debugCSRenderer->CreateDeviceDependentResources();

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

      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::VisibleLightLeftFront); //VIsible light must be enabled for depth to work.
      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::VisibleLightLeftLeft);
      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::VisibleLightRightFront);
      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::VisibleLightRightRight);
      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::LongThrowToFDepth);
      _holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::ShortThrowToFDepth);
      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::LongThrowToFReflectivity);
      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::ShortThrowToFReflectivity);
      //_holoLensMediaFrameSourceGroup->Enable(HoloLensForCV::SensorType::PhotoVideo);

      concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then(
         [&]()
      {
         _holoLensMediaFrameSourceGroupStarted = true;
      });
   }
}
