#pragma once

namespace HoloHands
{

   public ref class DepthSensor sealed
   {
   public:
      DepthSensor();

      Windows::Graphics::Imaging::SoftwareBitmap^ GetLatestBitmap() { return m_latestBitmap; };

   private:

      void FrameReader_FrameArrived(
         Windows::Media::Capture::Frames::MediaFrameReader^ sender,
         Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs^ args);

      concurrency::task<void> LoadMediaSourceAsync();
      Platform::String ^ GetKeyForSensor(
         Windows::Foundation::Collections::IMapView<Platform::String^, Windows::Media::Capture::Frames::MediaFrameSource^>^ frameSources,
         Platform::String ^ sensorName);
      concurrency::task<void> LoadMediaSourceWorkerAsync();
      concurrency::task<bool> TryInitializeMediaCaptureAsync(Windows::Media::Capture::Frames::MediaFrameSourceGroup^ group);
      concurrency::task<void> CleanupMediaCaptureAsync();

      void DebugOutputAllProperties(
         Windows::Foundation::Collections::IMapView<Platform::Guid,
         Platform::Object^>^ properties);

      bool GetSensorName(Windows::Media::Capture::Frames::MediaFrameSource^ source, Platform::String^& name);

      Platform::Agile<Windows::Media::Capture::MediaCapture> m_mediaCapture;
      int m_selectedSourceGroupIndex = 1;
      bool m_sensorsFound = false;

      Windows::Media::Capture::Frames::MediaFrameReader^ m_frameReader;
      Windows::Foundation::EventRegistrationToken m_frameArrivedToken;

      Windows::Graphics::Imaging::SoftwareBitmap^ m_latestBitmap;


      struct VolatileState
      {
         std::mutex m_mutex;

         // The currently selected source group.
         int m_selectedStreamId{ 1 };

         std::vector<std::pair<Windows::Media::Capture::Frames::MediaFrameReader^, Windows::Foundation::EventRegistrationToken>> m_readers;

         //std::map<Windows::Media::Capture::Frames::MediaFrameSourceKind, FrameRenderer^> m_frameRenderers;
         std::map<int, Platform::String^> m_names;
         std::map<int, int> m_FrameReadersToSourceIdMap;
         std::map<int, int> m_frameCount;

         // Setting this to false enabled displaying all the streams atlease for one frame
         bool m_firstRunComplete{ false };
      } m_volatileState;
   };
}