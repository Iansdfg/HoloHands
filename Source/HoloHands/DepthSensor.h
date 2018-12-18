#pragma once
namespace HoloHands
{
   public ref class DepthSensor sealed
   {
   public:
      DepthSensor();

      Windows::Graphics::Imaging::SoftwareBitmap^ GetImage();


      void FrameReader_FrameArrived(
         Windows::Media::Capture::Frames::MediaFrameReader^ sender,
         Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs^ args);
   private:
      concurrency::task<void> LoadMediaSourceAsync();
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

      struct VolatileState
      {
         std::mutex m_mutex;

         // The currently selected source group.
         int m_selectedStreamId{ 1 };

         std::vector<std::pair<Windows::Media::Capture::Frames::MediaFrameReader^, Windows::Foundation::EventRegistrationToken>> m_readers;

         //std::map<Windows::Media::Capture::Frames::MediaFrameSourceKind, FrameRenderer^> m_frameRenderers;
         //std::map<int, FrameRenderer^> m_frameRenderers;
         std::map<int, int> m_FrameReadersToSourceIdMap;
         std::map<int, int> m_frameCount;

         // Setting this to false enabled displaying all the streams atlease for one frame
         bool m_firstRunComplete{ false };
      } m_volatileState;
   };
}