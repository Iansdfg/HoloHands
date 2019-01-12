#include "pch.h"

#include "HoloHandsMain.h"

#include "Native/Rendering/DirectXHelper.h"
#include <windows.graphics.directx.direct3d11.interop.h>
#include <Collection.h>

using namespace HoloHands;

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Input::Spatial;
using namespace std::placeholders;

HoloHandsMain::HoloHandsMain(const std::shared_ptr<DeviceResources>& deviceResources)
   :
   m_deviceResources(deviceResources)
{
   m_deviceResources->RegisterDeviceNotify(this);
}

void HoloHandsMain::SetHolographicSpace(HolographicSpace^ holographicSpace)
{
   UnregisterHolographicEventHandlers();

   m_holographicSpace = holographicSpace;

   m_quadRenderer = std::make_unique<QuadRenderer>(m_deviceResources, Size(1268, 720));
   m_depthTexture = std::make_unique<DepthTexture>(m_deviceResources);
   m_depthSensor = std::make_unique<Sensor>(L"Short Throw ToF Depth");
   m_handDetector = std::make_unique<HandDetector>();

   m_quadRenderer->CreateDeviceDependentResources();
   m_depthTexture->CreateDeviceDependentResources();

   m_locator = SpatialLocator::GetDefault();

   m_locatabilityChangedToken =
      m_locator->LocatabilityChanged +=
      ref new Windows::Foundation::TypedEventHandler<SpatialLocator^, Object^>(
         std::bind(&HoloHandsMain::OnLocatabilityChanged, this, _1, _2)
         );

   m_cameraAddedToken =
      m_holographicSpace->CameraAdded +=
      ref new Windows::Foundation::TypedEventHandler<HolographicSpace^, HolographicSpaceCameraAddedEventArgs^>(
         std::bind(&HoloHandsMain::OnCameraAdded, this, _1, _2)
         );

   m_cameraRemovedToken =
      m_holographicSpace->CameraRemoved +=
      ref new Windows::Foundation::TypedEventHandler<HolographicSpace^, HolographicSpaceCameraRemovedEventArgs^>(
         std::bind(&HoloHandsMain::OnCameraRemoved, this, _1, _2)
         );

   m_referenceFrame = m_locator->CreateStationaryFrameOfReferenceAtCurrentLocation();

   m_attachedReferenceFrame = m_locator->CreateAttachedFrameOfReferenceAtCurrentHeading();
}

void HoloHandsMain::UnregisterHolographicEventHandlers()
{
   if (m_holographicSpace != nullptr)
   {
      // Clear previous event registrations.
      if (m_cameraAddedToken.Value != 0)
      {
         m_holographicSpace->CameraAdded -= m_cameraAddedToken;
         m_cameraAddedToken.Value = 0;
      }

      if (m_cameraRemovedToken.Value != 0)
      {
         m_holographicSpace->CameraRemoved -= m_cameraRemovedToken;
         m_cameraRemovedToken.Value = 0;
      }
   }

   if (m_locator != nullptr)
   {
      m_locator->LocatabilityChanged -= m_locatabilityChangedToken;
   }
}

HoloHandsMain::~HoloHandsMain()
{
   m_deviceResources->RegisterDeviceNotify(nullptr);

   UnregisterHolographicEventHandlers();
}

HolographicFrame^ HoloHandsMain::Update()
{
   HolographicFrame^ holographicFrame = m_holographicSpace->CreateNextFrame();

   HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

   m_deviceResources->EnsureCameraResources(holographicFrame, prediction);

   SpatialCoordinateSystem^ attachedCoordinateSystem = m_attachedReferenceFrame->GetStationaryCoordinateSystemAtTimestamp(prediction->Timestamp);
   
   m_timer.Tick([&]()
   {
      SpatialPointerPose^ pose = SpatialPointerPose::TryGetAtTimestamp(attachedCoordinateSystem, prediction->Timestamp);

      m_quadRenderer->UpdatePosition(pose);
      m_quadRenderer->Update(m_timer);
   });

   for (auto cameraPose : prediction->CameraPoses)
   {
      HolographicCameraRenderingParameters^ renderingParameters = holographicFrame->GetRenderingParameters(cameraPose);

      float3 focusPointPosition = m_quadRenderer->GetQuadPosition();
      float3 focusPointNormal = focusPointPosition == float3(0.f) ? float3(0.f, 0.f, 1.f) : -normalize(focusPointPosition);
      float3 focusPointVelocity = { 0, 0, 0 };

      renderingParameters->SetFocusPoint(
         attachedCoordinateSystem,
         focusPointPosition,
         focusPointNormal,
         focusPointVelocity);
   }

   return holographicFrame;
}

bool HoloHandsMain::Render(Windows::Graphics::Holographic::HolographicFrame^ holographicFrame)
{
   if (m_timer.GetFrameCount() == 0)
   {
      // Don't try to render anything before the first Update.
      return false;
   }

   // Lock the set of holographic camera resources, then draw to each camera in this frame.
   return m_deviceResources->UseHolographicCameraResources<bool>(
      [this, holographicFrame](std::map<UINT32, std::unique_ptr<CameraResources>>& cameraResourceMap)
   {
      holographicFrame->UpdateCurrentPrediction();
      HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

      bool atLeastOneCameraRendered = false;
      for (auto cameraPose : prediction->CameraPoses)
      {
         CameraResources* pCameraResources = cameraResourceMap[cameraPose->HolographicCamera->Id].get();

         const auto context = m_deviceResources->GetD3DDeviceContext();
         const auto depthStencilView = pCameraResources->GetDepthStencilView();

         ID3D11RenderTargetView *const targets[1] = { pCameraResources->GetBackBufferRenderTargetView() };
         context->OMSetRenderTargets(1, targets, depthStencilView);

         context->ClearRenderTargetView(targets[0], DirectX::Colors::Transparent); //DirectX::Colors::Red
         context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

         //pCameraResources->UpdateViewProjectionBuffer(m_deviceResources, cameraPose, m_referenceFrame->CoordinateSystem);
         pCameraResources->UpdateViewProjectionBuffer(
            m_deviceResources,
            cameraPose,
            m_attachedReferenceFrame->GetStationaryCoordinateSystemAtTimestamp(prediction->Timestamp));

         bool cameraActive = pCameraResources->AttachViewProjectionBuffer(m_deviceResources);

         if (cameraActive)
         {
            if (m_depthSensor->Updated())
            {
               m_depthSensor->Lock();

               auto bitmap = m_depthSensor->GetBitmap();

               cv::Mat handsMatrix;
               m_handDetector->Process(bitmap, handsMatrix);

               m_depthTexture->CopyFrom(bitmap); //TODO: pass in handsMatrix

               m_depthSensor->Unlock();
            }

            m_quadRenderer->Render(*m_depthTexture);
         }

         atLeastOneCameraRendered = true;
      }

      return atLeastOneCameraRendered;
   });
}

void HoloHandsMain::SaveAppState()
{
   //TODO:
}

void HoloHandsMain::LoadAppState()
{
   //TODO:
}

void HoloHandsMain::OnDeviceLost()
{
   m_quadRenderer->ReleaseDeviceDependentResources();
   m_depthTexture->ReleaseDeviceDependentResources();
}

void HoloHandsMain::OnDeviceRestored()
{
   m_quadRenderer->CreateDeviceDependentResources();
   m_depthTexture->CreateDeviceDependentResources();
}

void HoloHandsMain::OnLocatabilityChanged(SpatialLocator^ sender, Object^ args)
{
   switch (sender->Locatability)
   {
   case SpatialLocatability::Unavailable:
      // Holograms cannot be rendered.
   {
      String^ message = L"Warning! Positional tracking is " +
         sender->Locatability.ToString() + L".\n";
      OutputDebugStringW(message->Data());
   }
   break;

   // In the following three cases, it is still possible to place holograms using a
   // SpatialLocatorAttachedFrameOfReference.
   case SpatialLocatability::PositionalTrackingActivating:
      // The system is preparing to use positional tracking.

   case SpatialLocatability::OrientationOnly:
      // Positional tracking has not been activated.

   case SpatialLocatability::PositionalTrackingInhibited:
      // Positional tracking is temporarily inhibited. User action may be required
      // in order to restore positional tracking.
      break;

   case SpatialLocatability::PositionalTrackingActive:
      // Positional tracking is active. World-locked content can be rendered.
      break;
   }
}

void HoloHandsMain::OnCameraAdded(
   HolographicSpace^ sender,
   HolographicSpaceCameraAddedEventArgs^ args
)
{
   Deferral^ deferral = args->GetDeferral();
   HolographicCamera^ holographicCamera = args->Camera;
   create_task([this, deferral, holographicCamera]()
   {
      m_deviceResources->AddHolographicCamera(holographicCamera);

      deferral->Complete();
   });
}

void HoloHandsMain::OnCameraRemoved(
   HolographicSpace^ sender,
   HolographicSpaceCameraRemovedEventArgs^ args
)
{
   create_task([this]()
   {
      //
      // TODO: Asynchronously unload or deactivate content resources (not back buffer 
      //       resources) that are specific only to the camera that was removed.
      //
   });

   m_deviceResources->RemoveHolographicCamera(args->Camera);
}
