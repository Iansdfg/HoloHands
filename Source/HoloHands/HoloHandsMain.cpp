#include "pch.h"
#include "HoloHandsMain.h"
#include "Common\DirectXHelper.h"

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

// Loads and initializes application assets when the application is loaded.
HoloHandsMain::HoloHandsMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
   m_deviceResources(deviceResources)
{
   // Register to be notified if the device is lost or recreated.
   m_deviceResources->RegisterDeviceNotify(this);
}

void HoloHandsMain::SetHolographicSpace(HolographicSpace^ holographicSpace)
{
   UnregisterHolographicEventHandlers();

   m_holographicSpace = holographicSpace;


   m_spinningCubeRenderer = std::make_unique<SpinningCubeRenderer>(m_deviceResources);
   m_spatialInputHandler = std::make_unique<SpatialInputHandler>();

   m_quadRenderer = std::make_unique<QuadRenderer>(m_deviceResources);

   // Use the default SpatialLocator to track the motion of the device.
   m_locator = SpatialLocator::GetDefault();

   // Be able to respond to changes in the positional tracking state.
   m_locatabilityChangedToken =
      m_locator->LocatabilityChanged +=
      ref new Windows::Foundation::TypedEventHandler<SpatialLocator^, Object^>(
         std::bind(&HoloHandsMain::OnLocatabilityChanged, this, _1, _2)
         );

   // Respond to camera added events by creating any resources that are specific
   // to that camera, such as the back buffer render target view.
   // When we add an event handler for CameraAdded, the API layer will avoid putting
   // the new camera in new HolographicFrames until we complete the deferral we created
   // for that handler, or return from the handler without creating a deferral. This
   // allows the app to take more than one frame to finish creating resources and
   // loading assets for the new holographic camera.
   // This function should be registered before the app creates any HolographicFrames.
   m_cameraAddedToken =
      m_holographicSpace->CameraAdded +=
      ref new Windows::Foundation::TypedEventHandler<HolographicSpace^, HolographicSpaceCameraAddedEventArgs^>(
         std::bind(&HoloHandsMain::OnCameraAdded, this, _1, _2)
         );

   // Respond to camera removed events by releasing resources that were created for that
   // camera.
   // When the app receives a CameraRemoved event, it releases all references to the back
   // buffer right away. This includes render target views, Direct2D target bitmaps, and so on.
   // The app must also ensure that the back buffer is not attached as a render target, as
   // shown in DeviceResources::ReleaseResourcesForBackBuffer.
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
   // Deregister device notification.
   m_deviceResources->RegisterDeviceNotify(nullptr);

   UnregisterHolographicEventHandlers();
}

HolographicFrame^ HoloHandsMain::Update()
{
   HolographicFrame^ holographicFrame = m_holographicSpace->CreateNextFrame();

   HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

   m_deviceResources->EnsureCameraResources(holographicFrame, prediction);


   SpatialCoordinateSystem^ currentCoordinateSystem = m_referenceFrame->CoordinateSystem;

   SpatialInteractionSourceState^ pointerState = m_spatialInputHandler->CheckForInput();
   if (pointerState != nullptr)
   {
      m_spinningCubeRenderer->PositionHologram(
         pointerState->TryGetPointerPose(currentCoordinateSystem)
      );
   }

   //SpatialCoordinateSystem^ attachedCurrentCoordinateSystem =
   //   m_attachedReferenceFrame->GetStationaryCoordinateSystemAtTimestamp(prediction->Timestamp);

   ////Update quad position.
   //auto position = m_deviceResources->cam
   //m_quadRenderer->SetDirection(pose->Head->ForwardDirection);

   m_timer.Tick([&]()
   {
      m_spinningCubeRenderer->Update(m_timer);

      //Update quead renderer
      SpatialCoordinateSystem^ attachedCoordinateSystem = m_attachedReferenceFrame->GetStationaryCoordinateSystemAtTimestamp(prediction->Timestamp);
      SpatialPointerPose^ pose = SpatialPointerPose::TryGetAtTimestamp(attachedCoordinateSystem, prediction->Timestamp);

      m_quadRenderer->UpdatePosition(pose);
      m_quadRenderer->Update(m_timer);
   });

   for (auto cameraPose : prediction->CameraPoses)
   {
      HolographicCameraRenderingParameters^ renderingParameters = holographicFrame->GetRenderingParameters(cameraPose);

      //renderingParameters->SetFocusPoint(
      //   currentCoordinateSystem,
      //   m_spinningCubeRenderer->GetPosition()
      //);


      float3 const& focusPointPosition = m_quadRenderer->GetQuadPosition();
      float3        focusPointNormal = focusPointPosition == float3(0.f) ? float3(0.f, 0.f, 1.f) : -normalize(focusPointPosition);
      float3 const& focusPointVelocity = { 0, 0, 0 };


      SpatialCoordinateSystem^ coordinateSystemToUse = m_attachedReferenceFrame->GetStationaryCoordinateSystemAtTimestamp(prediction->Timestamp);
      renderingParameters->SetFocusPoint(
         coordinateSystemToUse,
         focusPointPosition,
         focusPointNormal,
         focusPointVelocity);
   }

   // The holographic frame will be used to get up-to-date view and projection matrices and
   // to present the swap chain.
   return holographicFrame;
}

// Renders the current frame to each holographic camera, according to the
// current application and spatial positioning state. Returns true if the
// frame was rendered to at least one camera.
bool HoloHandsMain::Render(Windows::Graphics::Holographic::HolographicFrame^ holographicFrame)
{
   // Don't try to render anything before the first Update.
   if (m_timer.GetFrameCount() == 0)
   {
      return false;
   }

   // Lock the set of holographic camera resources, then draw to each camera
   // in this frame.
   return m_deviceResources->UseHolographicCameraResources<bool>(
      [this, holographicFrame](std::map<UINT32, std::unique_ptr<DX::CameraResources>>& cameraResourceMap)
   {
      holographicFrame->UpdateCurrentPrediction();
      HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

      bool atLeastOneCameraRendered = false;
      for (auto cameraPose : prediction->CameraPoses)
      {
         DX::CameraResources* pCameraResources = cameraResourceMap[cameraPose->HolographicCamera->Id].get();

         const auto context = m_deviceResources->GetD3DDeviceContext();
         const auto depthStencilView = pCameraResources->GetDepthStencilView();

         ID3D11RenderTargetView *const targets[1] = { pCameraResources->GetBackBufferRenderTargetView() };
         context->OMSetRenderTargets(1, targets, depthStencilView);

         context->ClearRenderTargetView(targets[0], DirectX::Colors::Transparent);
         context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

         //pCameraResources->UpdateViewProjectionBuffer(m_deviceResources, cameraPose, m_referenceFrame->CoordinateSystem);
         pCameraResources->UpdateViewProjectionBuffer(
            m_deviceResources,
            cameraPose,
            m_attachedReferenceFrame->GetStationaryCoordinateSystemAtTimestamp(prediction->Timestamp));

         bool cameraActive = pCameraResources->AttachViewProjectionBuffer(m_deviceResources);

         if (cameraActive)
         {
            m_spinningCubeRenderer->Render();

            m_quadRenderer->Render();
         }

         atLeastOneCameraRendered = true;
      }

      return atLeastOneCameraRendered;
   });
}

void HoloHandsMain::SaveAppState()
{
   //
   // TODO: Insert code here to save your app state.
   //       This method is called when the app is about to suspend.
   //
   //       For example, store information in the SpatialAnchorStore.
   //
}

void HoloHandsMain::LoadAppState()
{
   //
   // TODO: Insert code here to load your app state.
   //       This method is called when the app resumes.
   //
   //       For example, load information from the SpatialAnchorStore.
   //
}

void HoloHandsMain::OnDeviceLost()
{
   m_spinningCubeRenderer->ReleaseDeviceDependentResources();
   m_quadRenderer->ReleaseDeviceDependentResources();
}

void HoloHandsMain::OnDeviceRestored()
{
   m_spinningCubeRenderer->CreateDeviceDependentResources();

   m_quadRenderer->CreateDeviceDependentResources();
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
