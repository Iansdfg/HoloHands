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

        Windows::UI::Input::Spatial::SpatialInteractionManager^     m_interactionManager;

        Windows::Foundation::EventRegistrationToken m_sourcePressedEventToken;
        Windows::Foundation::EventRegistrationToken m_sourceReleasedEventToken;
        Windows::Foundation::EventRegistrationToken m_sourceUpdatedEventToken;
        Windows::Foundation::EventRegistrationToken m_sourceLostEventToken;
        Windows::Foundation::EventRegistrationToken m_sourceDetectedEventToken;

        Windows::UI::Input::Spatial::SpatialInteractionSourceState^ m_sourceState = nullptr;
    };
}
