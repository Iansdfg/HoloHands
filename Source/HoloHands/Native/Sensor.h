#pragma once

namespace HoloHands
{
   class DepthTexture;

   class Sensor
   {
   public:
      Sensor(const std::wstring& sensorName);

      Windows::Graphics::Imaging::SoftwareBitmap^ GetBitmap();
      void Lock();
      void Unlock();
      bool BitmapIsDirty();

   private:
      static Platform::String^ GetSensorName(Windows::Media::Capture::Frames::MediaFrameSource^ source);

      static Windows::Media::Capture::Frames::MediaFrameSourceGroup^ GetSensorStreamingGroup(
         Windows::Foundation::Collections::IVectorView<Windows::Media::Capture::Frames::MediaFrameSourceGroup^>^ groups);

      static Windows::Media::Capture::Frames::MediaFrameSource^ GetMediaFrameSourceFromMap(
         Windows::Foundation::Collections::IMapView<Platform::String^, Windows::Media::Capture::Frames::MediaFrameSource^>^ frameSources,
         const std::wstring& sensorName);

      static Platform::String^ GetSubtypeForFormat(
         Windows::Media::Capture::Frames::MediaFrameSourceKind kind,
         Windows::Media::Capture::Frames::MediaFrameFormat^ format);

      static Windows::Media::Capture::Frames::MediaFrameFormat^ GetMediaFrameFormat(
         Windows::Foundation::Collections::IVectorView<Windows::Media::Capture::Frames::MediaFrameFormat^>^ formats,
         Windows::Media::Capture::Frames::MediaFrameSourceKind kind);

      concurrency::task<void> LoadMediaSourceAsync();
      concurrency::task<void> LoadMediaSourceWorkerAsync();
      concurrency::task<bool> TryInitializeMediaCaptureAsync(Windows::Media::Capture::Frames::MediaFrameSourceGroup^ group);
      concurrency::task<void> CleanupMediaCaptureAsync();

      void FrameReader_FrameArrived(
         Windows::Media::Capture::Frames::MediaFrameReader^ sender,
         Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs^ args);

      Platform::Agile<Windows::Media::Capture::MediaCapture> m_mediaCapture;
      Windows::Media::Capture::Frames::MediaFrameReader^ m_frameReader;
      Windows::Foundation::EventRegistrationToken m_frameArrivedToken;

      Windows::Graphics::Imaging::SoftwareBitmap^ m_bitmap;
      std::wstring m_sensorName;
      bool m_bitmapIsDirty;
      std::mutex m_preventStateChanges;
   };
}