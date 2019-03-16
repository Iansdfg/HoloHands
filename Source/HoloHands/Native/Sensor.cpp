#include "pch.h"

#include "Sensor.h"

#include "Io/All.h"
#include <unordered_set>
#include <sstream>

using namespace HoloHands;
using namespace Concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Capture;
using namespace Windows::Media::Capture::Frames;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Storage;
using namespace std::placeholders;

EXTERN_GUID(MF_MT_USER_DATA, 0xb6bc765f, 0x4c3b, 0x40a4, 0xbd, 0x51, 0x25, 0x35, 0xb6, 0x6f, 0xe0, 0x9d);

#define CURRENT_CONTEXT task_continuation_context::get_current_winrt_context()

Sensor::Sensor(const std::wstring& sensorName)
   :
   _sensorName(sensorName),
   _bitmapIsDirty(false)
{
   LoadMediaSourceWorkerAsync()
      .then([this]()
   {
   }, task_continuation_context::use_current());
}

SoftwareBitmap^ Sensor::GetBitmap()
{
   _bitmapIsDirty = false;
   return _bitmap;
}

void Sensor::Lock()
{
   _preventStateChanges.lock();
}

void Sensor::Unlock()
{
   _preventStateChanges.unlock();
}

bool Sensor::Updated()
{
   return _bitmapIsDirty;
}

String^ Sensor::GetSensorName(Windows::Media::Capture::Frames::MediaFrameSource^ source)
{
   if (source->Info->Properties->HasKey(MF_MT_USER_DATA))
   {
      Object^ mfMtUserData =
         source->Info->Properties->Lookup(
            MF_MT_USER_DATA);

      Array<byte>^ sensorNameAsPlatformArray =
         safe_cast<IBoxArray<byte>^>(
            mfMtUserData)->Value;

      return ref new String(
         reinterpret_cast<const wchar_t*>(
            sensorNameAsPlatformArray->Data));
   }

   return nullptr;
}

MediaFrameSourceGroup^ Sensor::GetSensorStreamingGroup(IVectorView<MediaFrameSourceGroup^>^ groups)
{
   for (uint32_t i = 0; i < groups->Size; ++i)
   {
      MediaFrameSourceGroup^ candidateGroup = groups->GetAt(i);

      if (candidateGroup->DisplayName == "Sensor Streaming")
      {
         return candidateGroup;
      }
   }

   return nullptr;
}

MediaFrameSource^ Sensor::GetMediaFrameSourceFromMap(IMapView<String^, MediaFrameSource^>^ frameSources, const std::wstring& sensorName)
{
   for (IKeyValuePair<String^, MediaFrameSource^>^ kvp : frameSources)
   {
      MediaFrameSource^ source = kvp->Value;
      MediaFrameSourceKind kind = source->Info->SourceKind;

      String^ currentSensorName = GetSensorName(source);

      if (currentSensorName->Data() == sensorName)
      {
         return kvp->Value;
      }
   }

   return nullptr;
}

String^ Sensor::GetSubtypeForFormat(MediaFrameSourceKind kind, MediaFrameFormat^ format)
{
   // Note that media encoding subtypes may differ in case.
   // https://docs.microsoft.com/en-us/uwp/api/Windows.Media.MediaProperties.MediaEncodingSubtypes

   String^ subtype = format->Subtype;
   switch (kind)
   {
      // For color sources, we accept anything and request that it be converted to Bgra8.
   case MediaFrameSourceKind::Color:
      return Windows::Media::MediaProperties::MediaEncodingSubtypes::Bgra8;

      // The only depth format we can render is D16.
   case MediaFrameSourceKind::Depth:
      return CompareStringOrdinal(subtype->Data(), -1, L"D16", -1, TRUE) == CSTR_EQUAL ? subtype : nullptr;

      // The only infrared formats we can render are L8 and L16.
   case MediaFrameSourceKind::Infrared:
      return (CompareStringOrdinal(subtype->Data(), -1, L"L8", -1, TRUE) == CSTR_EQUAL ||
         CompareStringOrdinal(subtype->Data(), -1, L"D16", -1, TRUE) == CSTR_EQUAL) ? subtype : nullptr;

      // No other source kinds are supported by this class.
   default:
      return nullptr;
   }
}

MediaFrameFormat^ Sensor::GetMediaFrameFormat(IVectorView<MediaFrameFormat^>^ formats, MediaFrameSourceKind kind)
{
   auto found = std::find_if(begin(formats), end(formats),
      [&](MediaFrameFormat^ format)
   {
      return GetSubtypeForFormat(kind, format) != nullptr;
   });

   if (found != end(formats))
   {
      return *found;
   }

   return nullptr;
}

task<void> Sensor::LoadMediaSourceAsync()
{
   return LoadMediaSourceWorkerAsync()
      .then([this]()
   {
   }, task_continuation_context::use_current());
}

task<void> Sensor::LoadMediaSourceWorkerAsync()
{
   return CleanupMediaCaptureAsync()
      .then([this]()
   {
      return create_task(MediaFrameSourceGroup::FindAllAsync());
   }, CURRENT_CONTEXT)
      .then([this](IVectorView<MediaFrameSourceGroup^>^ allGroups)
   {
      MediaFrameSourceGroup^ sensorStreamingGroup = GetSensorStreamingGroup(allGroups);
      if (!sensorStreamingGroup)
      {
         OutputDebugString(L"No Sensor Streaming groups found.");
         return task_from_result();
      }

      return TryInitializeMediaCaptureAsync(sensorStreamingGroup)
         .then([this](bool initialized)
      {
         if (!initialized)
         {
            return CleanupMediaCaptureAsync();
         }

         MediaFrameSource^ source = GetMediaFrameSourceFromMap(_mediaCapture->FrameSources, _sensorName);

         return create_task([this, source]()
         {
            MediaFrameFormat^ format = GetMediaFrameFormat(source->SupportedFormats, source->Info->SourceKind);

            return create_task(source->SetFormatAsync(format))
               .then([this, source, format]()
            {
               String^ subtype = GetSubtypeForFormat(source->Info->SourceKind, format);

               return create_task(_mediaCapture->CreateFrameReaderAsync(source, subtype));
            }, CURRENT_CONTEXT)
               .then([this, source](MediaFrameReader^ frameReader)
            {
               std::lock_guard<std::mutex> lockGuard(_preventStateChanges);

               _frameArrivedToken = frameReader->FrameArrived +=
                  ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(
                     std::bind(&Sensor::FrameReader_FrameArrived, this, _1, _2));

               _frameReader = frameReader;

               OutputDebugString(L"Reader created.");

               return create_task(frameReader->StartAsync());
            }, CURRENT_CONTEXT)
               .then([this](MediaFrameReaderStartStatus status)
            {
               if (status == MediaFrameReaderStartStatus::Success)
               {
                  OutputDebugString(L"Started reader.");
               }
               else
               {
                  OutputDebugString(("Unable to start reader. Error: " + status.ToString())->Data());
               }
            }, CURRENT_CONTEXT);
         }, CURRENT_CONTEXT);
      }, CURRENT_CONTEXT);
   }, CURRENT_CONTEXT);
}

task<bool> Sensor::TryInitializeMediaCaptureAsync(MediaFrameSourceGroup^ group)
{
   if (_mediaCapture != nullptr)
   {
      // Already initialized.
      return task_from_result(true);
   }

   _mediaCapture = ref new MediaCapture();

   auto settings = ref new MediaCaptureInitializationSettings();
   settings->SourceGroup = group;
   settings->SharingMode = MediaCaptureSharingMode::SharedReadOnly;
   settings->StreamingCaptureMode = StreamingCaptureMode::Video;
   settings->MemoryPreference = MediaCaptureMemoryPreference::Cpu;
   settings->StreamingCaptureMode = StreamingCaptureMode::Video;

   return create_task(_mediaCapture->InitializeAsync(settings))
      .then([this](task<void> initializeMediaCaptureTask)
   {
      try
      {
         initializeMediaCaptureTask.get();
         OutputDebugString(L"MediaCapture is successfully initialized in shared mode.\n");
         return true;
      }
      catch (Exception^ exception)
      {
         OutputDebugString(("Failed to initialize media capture: " + exception->Message + "\n")->Data());
         return false;
      }
   });
}

task<void> Sensor::CleanupMediaCaptureAsync()
{
   task<void> cleanupTask = task_from_result();

   if (_mediaCapture != nullptr)
   {
      _frameReader->FrameArrived -= _frameArrivedToken;
      cleanupTask = cleanupTask && create_task(_frameReader->StopAsync());

      cleanupTask = cleanupTask.then([this] {
         OutputDebugString(L"Cleaning up MediaCapture...\n");
         _mediaCapture = nullptr;
      });
   }
   return cleanupTask;
}

void Sensor::FrameReader_FrameArrived(MediaFrameReader^ sender, MediaFrameArrivedEventArgs^ args)
{
   if (sender == nullptr)
   {
      return;
   }

   if (MediaFrameReference^ frame = sender->TryAcquireLatestFrame())
   {
      if (frame != nullptr)
      {
         std::lock_guard<std::mutex> lockGuard(_preventStateChanges);

         _bitmap = frame->VideoMediaFrame->SoftwareBitmap;
         _bitmapIsDirty = true;
      }
   }
}
