#pragma once

#include "Native/Rendering/DeviceResources.h"
#include "Native/Rendering/QuadRenderer.h"
#include "Native/Utils/StepTimer.h"
#include "Native/DepthTexture.h"  

#include "Managed/DepthSensor.h"

namespace HoloHands
{
    class HoloHandsMain : public DX::IDeviceNotify
    {
    public:
        HoloHandsMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        ~HoloHandsMain();

        // Sets the holographic space. This is our closest analogue to setting a new window
        // for the app.
        void SetHolographicSpace(Windows::Graphics::Holographic::HolographicSpace^ holographicSpace);

        // Starts the holographic frame and updates the content.
        Windows::Graphics::Holographic::HolographicFrame^ Update();

        // Renders holograms, including world-locked content.
        bool Render(Windows::Graphics::Holographic::HolographicFrame^ holographicFrame);

        // Handle saving and loading of app state owned by AppMain.
        void SaveAppState();
        void LoadAppState();

        // IDeviceNotify
        virtual void OnDeviceLost();
        virtual void OnDeviceRestored();

    private:
        // Asynchronously creates resources for new holographic cameras.
        void OnCameraAdded(
            Windows::Graphics::Holographic::HolographicSpace^ sender,
            Windows::Graphics::Holographic::HolographicSpaceCameraAddedEventArgs^ args);

        // Synchronously releases resources for holographic cameras that are no longer
        // attached to the system.
        void OnCameraRemoved(
            Windows::Graphics::Holographic::HolographicSpace^ sender,
            Windows::Graphics::Holographic::HolographicSpaceCameraRemovedEventArgs^ args);

        // Used to notify the app when the positional tracking state changes.
        void OnLocatabilityChanged(
            Windows::Perception::Spatial::SpatialLocator^ sender,
            Platform::Object^ args);

        // Clears event registration state. Used when changing to a new HolographicSpace
        // and when tearing down AppMain.
        void UnregisterHolographicEventHandlers();

        std::unique_ptr<QuadRenderer> m_quadRenderer;

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // Render loop timer.
        DX::StepTimer m_timer;

        // Represents the holographic space around the user.
        Windows::Graphics::Holographic::HolographicSpace^ m_holographicSpace;

        // SpatialLocator that is attached to the primary camera.
        Windows::Perception::Spatial::SpatialLocator^ m_locator;

        Windows::Perception::Spatial::SpatialStationaryFrameOfReference^ m_referenceFrame;
        Windows::Perception::Spatial::SpatialLocatorAttachedFrameOfReference^ m_attachedReferenceFrame;

        // Event registration tokens.
        Windows::Foundation::EventRegistrationToken m_cameraAddedToken;
        Windows::Foundation::EventRegistrationToken m_cameraRemovedToken;
        Windows::Foundation::EventRegistrationToken m_locatabilityChangedToken;

        DepthSensor^ m_depthSensor = nullptr;
        std::unique_ptr<DepthTexture> m_depthTexture = nullptr;
    };
}
