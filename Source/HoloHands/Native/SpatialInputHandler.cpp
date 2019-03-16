#include "pch.h"
#include "SpatialInputHandler.h"
#include <functional>

using namespace HoloHands;

using namespace Windows::Foundation;
using namespace Windows::UI::Input::Spatial;
using namespace std::placeholders;

SpatialInputHandler::SpatialInputHandler()
{
   _interactionManager = SpatialInteractionManager::GetForCurrentView();

   _sourcePressedEventToken =
      _interactionManager->SourcePressed +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   _sourceReleasedEventToken =
      _interactionManager->SourceReleased +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   _sourceUpdatedEventToken =
      _interactionManager->SourceUpdated +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   _sourceLostEventToken =
      _interactionManager->SourceLost +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));

   _sourceDetectedEventToken =
      _interactionManager->SourceDetected +=
      ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionSourceEventArgs^>(
         bind(&SpatialInputHandler::OnSourceChanged, this, _1, _2));
}

SpatialInputHandler::~SpatialInputHandler()
{
   _interactionManager->SourcePressed -= _sourcePressedEventToken;
   _interactionManager->SourceReleased -= _sourceReleasedEventToken;
   _interactionManager->SourceUpdated -= _sourceUpdatedEventToken;
   _interactionManager->SourceLost -= _sourceLostEventToken;
   _interactionManager->SourceDetected -= _sourceDetectedEventToken;
}

SpatialInteractionSourceState^ SpatialInputHandler::CheckForInput()
{
   SpatialInteractionSourceState^ sourceState = _sourceState;
   _sourceState = nullptr;
   return sourceState;
}

void SpatialInputHandler::OnSourceChanged(SpatialInteractionManager^ sender, SpatialInteractionSourceEventArgs^ args)
{
   _sourceState = args->State;
}
