#include "pch.h"
#include "SpatialInputHandler.h"
#include <functional>

using namespace HoloHands;

using namespace Windows::Foundation;
using namespace Windows::UI::Input::Spatial;
using namespace std::placeholders;

SpatialInputHandler::SpatialInputHandler()
{
   m_interactionManager = SpatialInteractionManager::GetForCurrentView();

   m_sourcePressedEventToken =
      m_interactionManager->SourcePressed +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   m_sourceReleasedEventToken =
      m_interactionManager->SourceReleased +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   m_sourceUpdatedEventToken =
      m_interactionManager->SourceUpdated +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   m_sourceLostEventToken =
      m_interactionManager->SourceLost +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   m_sourceDetectedEventToken =
      m_interactionManager->SourceDetected +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));
}

SpatialInputHandler::~SpatialInputHandler()
{
   m_interactionManager->SourcePressed -= m_sourcePressedEventToken;
   m_interactionManager->SourceReleased -= m_sourceReleasedEventToken;
   m_interactionManager->SourceUpdated -= m_sourceUpdatedEventToken;
   m_interactionManager->SourceLost -= m_sourceLostEventToken;
   m_interactionManager->SourceDetected -= m_sourceDetectedEventToken;
}

SpatialInteractionSourceState^ SpatialInputHandler::CheckForInput()
{
   SpatialInteractionSourceState^ sourceState = m_sourceState;
   m_sourceState = nullptr;
   return sourceState;
}

void SpatialInputHandler::OnSourceChanged(SpatialInteractionManager^ sender, SpatialInteractionSourceEventArgs^ args)
{
   m_sourceState = args->State;
}
