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
   };
}