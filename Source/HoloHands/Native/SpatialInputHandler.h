#pragma once

namespace HoloHands
{
    class SpatialInputHandler
    {
    public:
        SpatialInputHandler();
        ~SpatialInputHandler();

        Windows::UI::Input::Spatial::SpatialInteractionSourceState^ CheckForInput();

    private:
        void OnSourceChanged(
            Windows::UI::Input::Spatial::SpatialInteractionManager^ sender,
            Windows::UI::Input::Spatial::SpatialInteractionSourceEventArgs^ args);

        Windows::UI::Input::Spatial::SpatialInteractionManager^     _interactionManager;

        Windows::Foundation::EventRegistrationToken _sourcePressedEventToken;
        Windows::Foundation::EventRegistrationToken _sourceReleasedEventToken;
        Windows::Foundation::EventRegistrationToken _sourceUpdatedEventToken;
        Windows::Foundation::EventRegistrationToken _sourceLostEventToken;
        Windows::Foundation::EventRegistrationToken _sourceDetectedEventToken;

        Windows::UI::Input::Spatial::SpatialInteractionSourceState^ _sourceState = nullptr;
    };
}
