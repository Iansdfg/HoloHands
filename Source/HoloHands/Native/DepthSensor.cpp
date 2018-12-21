#include "pch.h"

#include "DepthSensor.h"

#include "Native/DepthTexture.h"
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

DepthSensor::DepthSensor(DepthTexture& texture)
   :
   m_texture(texture)
{
   LoadMediaSourceWorkerAsync()
      .then([this]()
   {
   }, task_continuation_context::use_current());
}

Windows::Graphics::Imaging::SoftwareBitmap ^ HoloHands::DepthSensor::GetLatestBitmap()
{
   m_hasNewBitmap = false;
   return m_latestBitmap;
}

void HoloHands::DepthSensor::Lock()
{
   m_readingBitmap.lock();
}

void HoloHands::DepthSensor::Unlock()
{
   m_readingBitmap.unlock();
}

bool HoloHands::DepthSensor::HasNewBitmap()
{
   return m_hasNewBitmap;
}


EXTERN_GUID(MFSampleExtension_DeviceTimestamp, 0x8f3e35e7, 0x2dcd, 0x4887, 0x86, 0x22, 0x2a, 0x58, 0xba, 0xa6, 0x52, 0xb0);
EXTERN_GUID(MFSampleExtension_Spatial_CameraViewTransform, 0x4e251fa4, 0x830f, 0x4770, 0x85, 0x9a, 0x4b, 0x8d, 0x99, 0xaa, 0x80, 0x9b);
EXTERN_GUID(MFSampleExtension_Spatial_CameraCoordinateSystem, 0x9d13c82f, 0x2199, 0x4e67, 0x91, 0xcd, 0xd1, 0xa4, 0x18, 0x1f, 0x25, 0x34);
EXTERN_GUID(MFSampleExtension_Spatial_CameraProjectionTransform, 0x47f9fcb5, 0x2a02, 0x4f26, 0xa4, 0x77, 0x79, 0x2f, 0xdf, 0x95, 0x88, 0x6a);
EXTERN_GUID(MFSampleExtension_DeviceReferenceSystemTime, 0x6523775a, 0xba2d, 0x405f, 0xb2, 0xc5, 0x01, 0xff, 0x88, 0xe2, 0xe8, 0xf6);
EXTERN_GUID(MFSampleExtension_CameraExtrinsics, 0x6b761658, 0xb7ec, 0x4c3b, 0x82, 0x25, 0x86, 0x23, 0xca, 0xbe, 0xc3, 0x1d);
EXTERN_GUID(MF_MT_USER_DATA, 0xb6bc765f, 0x4c3b, 0x40a4, 0xbd, 0x51, 0x25, 0x35, 0xb6, 0x6f, 0xe0, 0x9d);
EXTERN_GUID(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, 0x17145fd1, 0x1b2b, 0x423c, 0x80, 0x1, 0x2b, 0x68, 0x33, 0xed, 0x35, 0x88);

static Platform::String^ PropertyGuidToString(Platform::Guid& guid)
{
   if (guid == Platform::Guid(MFSampleExtension_DeviceTimestamp))
   {
      return L"MFSampleExtension_DeviceTimestamp";
   }
   else if (guid == Platform::Guid(MFSampleExtension_Spatial_CameraViewTransform))
   {
      return L"MFSampleExtension_Spatial_CameraViewTransform";
   }
   else if (guid == Platform::Guid(MFSampleExtension_Spatial_CameraCoordinateSystem))
   {
      return L"MFSampleExtension_Spatial_CameraCoordinateSystem";
   }
   else if (guid == Platform::Guid(MFSampleExtension_Spatial_CameraProjectionTransform))
   {
      return L"MFSampleExtension_Spatial_CameraProjectionTransform";
   }
   else if (guid == Platform::Guid(MFSampleExtension_DeviceReferenceSystemTime))
   {
      return L"MFSampleExtension_DeviceReferenceSystemTime";
   }
   else if (guid == Platform::Guid(MFSampleExtension_CameraExtrinsics))
   {
      return L"MFSampleExtension_CameraExtrinsics";
   }
   else if (guid == Platform::Guid(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES))
   {
      return L"MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES";
   }
   else if (guid == Platform::Guid(MF_MT_USER_DATA))
   {
      return L"MF_MT_USER_DATA";
   }
   else
   {
      return guid.ToString();
   }
}

int MFSourceIdToStreamId(const std::wstring& sourceIdStr)
{
   size_t start = sourceIdStr.find_first_of(L'#');
   size_t end = sourceIdStr.find_first_of(L'@');

   assert(start != std::wstring::npos);
   assert(end != std::wstring::npos);
   assert(end > start);

   std::wstring idStr = sourceIdStr.substr(start + 1, end - start - 1);
   assert(idStr.size() != 0);

   std::wstringstream wss(idStr);
   int id = 0;

   assert(wss >> id);
   return id;
}

void DepthSensor::DebugOutputAllProperties(Windows::Foundation::Collections::IMapView<Platform::Guid, Platform::Object^>^ properties)
{
   if (IsDebuggerPresent())
   {
      auto itr = properties->First();
      while (itr->HasCurrent)
      {
         auto current = itr->Current;

         auto keyString = PropertyGuidToString(current->Key);
         OutputDebugStringW(reinterpret_cast<const wchar_t*>(keyString->Data()));
         OutputDebugStringW(L"\n");
         itr->MoveNext();
      }
   }
}

bool DepthSensor::GetSensorName(
   Windows::Media::Capture::Frames::MediaFrameSource^ source,
   Platform::String^& name)
{
   bool success = false;
   if (source->Info->Properties->HasKey(MF_MT_USER_DATA))
   {

      Platform::Object^ mfMtUserData =
         source->Info->Properties->Lookup(
            MF_MT_USER_DATA);

      Platform::Array<byte>^ sensorNameAsPlatformArray =
         safe_cast<Platform::IBoxArray<byte>^>(
            mfMtUserData)->Value;

      name = ref new Platform::String(
         reinterpret_cast<const wchar_t*>(
            sensorNameAsPlatformArray->Data));

      success = true;
   }
   return success;
}

task<void> DepthSensor::LoadMediaSourceAsync()
{
   return LoadMediaSourceWorkerAsync()
      .then([this]()
   {
   }, task_continuation_context::use_current());
}

String^ DepthSensor::GetKeyForSensor(IMapView<String^, MediaFrameSource^>^ frameSources, String^ sensorName)
{
   OutputDebugString(L"Sensors:\n");
   for (IKeyValuePair<String^, MediaFrameSource^>^ kvp : frameSources)
   {
      MediaFrameSource^ source = kvp->Value;
      MediaFrameSourceKind kind = source->Info->SourceKind;

      std::wstring sourceIdStr(source->Info->Id->Data());
      int id = MFSourceIdToStreamId(sourceIdStr);

      Platform::String^ currentSensorName;

      GetSensorName(source, currentSensorName);

      OutputDebugString((currentSensorName + "\n")->Data());

      if (currentSensorName == sensorName)
      {
         return kvp->Key;
      }
   }

   return nullptr;
}

String^ GetSubtypeForFrameReader(MediaFrameSourceKind kind, MediaFrameFormat^ format)
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

task<void> DepthSensor::LoadMediaSourceWorkerAsync()
{
   return CleanupMediaCaptureAsync()
      .then([this]()
   {
      return create_task(MediaFrameSourceGroup::FindAllAsync());
   }, task_continuation_context::get_current_winrt_context())
      .then([this](IVectorView<MediaFrameSourceGroup^>^ allGroups)
   {
      if (allGroups->Size == 0)
      {
         OutputDebugString(L"No source groups found.");
         return task_from_result();
      }

      MediaFrameSourceGroup^ selectedGroup;

      for (uint32_t i = 0; i < allGroups->Size; ++i)
      {
         MediaFrameSourceGroup^ candidateGroup = allGroups->GetAt(i);

         if (candidateGroup->DisplayName == "Sensor Streaming")
         {
            m_selectedSourceGroupIndex = i;
            selectedGroup = candidateGroup;
            break;
         }
      }

      if (!selectedGroup)
      {
         OutputDebugString(L"No Sensor Streaming groups found.");
         return task_from_result();
      }

      OutputDebugString(("Found " + allGroups->Size.ToString() + " groups and " +
         "selecting index [" + m_selectedSourceGroupIndex.ToString() + "] : " +
         selectedGroup->DisplayName)->Data());

      // Initialize MediaCapture with selected group.
      return TryInitializeMediaCaptureAsync(selectedGroup)
         .then([this, selectedGroup](bool initialized)
      {
         if (!initialized)
         {
            return CleanupMediaCaptureAsync();
         }

         // Set up frame readers, register event handlers and start streaming.
         auto startedKinds = std::make_shared<std::unordered_set<MediaFrameSourceKind>>();
         task<void> createReadersTask = task_from_result();


         //String^ selectedSensorName = ref new String(L"Short Throw ToF Depth");
         ////String^ selectedSensorName = ref new String(L"Visible Light Left-Left");

         //String^ selectedSensorKey = GetKeyForSensor(m_mediaCapture->FrameSources, selectedSensorName);

         //bool containsKey = m_mediaCapture->FrameSources->HasKey(selectedSensorKey);
         //if (!containsKey)
         //{
         //   OutputDebugString(("Error: No sensor found with the name: " + selectedSensorName + "\n")->Data());
         //   return task_from_result();
         //}

         for (IKeyValuePair<String^, MediaFrameSource^>^ kvp : m_mediaCapture->FrameSources)
         {

            std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);
            MediaFrameSource^ source = kvp->Value;

            MediaFrameSourceKind kind = source->Info->SourceKind;

            std::wstring sourceIdStr(source->Info->Id->Data());
            int id = MFSourceIdToStreamId(sourceIdStr);

            Platform::String^ sensorName(kind.ToString());

#if DEBUG_PRINT_PROPERTIES 
            DebugOutputAllProperties(source->Info->Properties);
#endif

            GetSensorName(source, sensorName);;

            m_volatileState.m_names[id] = sensorName;

            // Read all frames the first time
            if (m_volatileState.m_firstRunComplete && (id != m_volatileState.m_selectedStreamId))
            {
               return task_from_result();
            }

            createReadersTask = createReadersTask.then([this, startedKinds, source, kind, id]()
            {
               String^ requestedSubtype = nullptr;
               auto found = std::find_if(begin(source->SupportedFormats), end(source->SupportedFormats),
                  [&](MediaFrameFormat^ format)
               {
                  requestedSubtype = GetSubtypeForFrameReader(kind, format);
                  return requestedSubtype != nullptr;
               });
               if (requestedSubtype == nullptr)
               {
                  // No acceptable format was found. Ignore this source.
                  return task_from_result();
               }

               // Tell the source to use the format we can render.
               return create_task(source->SetFormatAsync(*found))
                  .then([this, source, requestedSubtype]()
               {
                  return create_task(m_mediaCapture->CreateFrameReaderAsync(source, requestedSubtype));
               }, task_continuation_context::get_current_winrt_context())
                  .then([this, kind, source, id](MediaFrameReader^ frameReader)
               {
                  std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);

                  // Add frame reader to the internal lookup
                  m_volatileState.m_FrameReadersToSourceIdMap[frameReader->GetHashCode()] = id;

                  EventRegistrationToken token = frameReader->FrameArrived +=
                     ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(
                        std::bind(&DepthSensor::FrameReader_FrameArrived, this, _1, _2));

                  // Keep track of created reader and event handler so it can be stopped later.
                  m_volatileState.m_readers.push_back(std::make_pair(frameReader, token));

                  OutputDebugString((kind.ToString() + " reader created")->Data());

                  return create_task(frameReader->StartAsync());
               }, task_continuation_context::get_current_winrt_context())
                  .then([this, kind, startedKinds](MediaFrameReaderStartStatus status)
               {
                  if (status == MediaFrameReaderStartStatus::Success)
                  {
                     OutputDebugString(("Started " + kind.ToString() + " reader.")->Data());
                     startedKinds->insert(kind);
                  }
                  else
                  {
                     OutputDebugString(("Unable to start " + kind.ToString() + "  reader. Error: " + status.ToString())->Data());
                  }
               }, task_continuation_context::get_current_winrt_context());
            }, task_continuation_context::get_current_winrt_context());
         }
         // Run the loop and see if any sources were used.
         return createReadersTask.then([this, startedKinds, selectedGroup]()
         {
            if (startedKinds->size() == 0)
            {
               OutputDebugString(("No eligible sources in " + selectedGroup->DisplayName + ".")->Data());
            }
         }, task_continuation_context::get_current_winrt_context());
      }, task_continuation_context::get_current_winrt_context());
   }, task_continuation_context::get_current_winrt_context());
}

task<bool> DepthSensor::TryInitializeMediaCaptureAsync(MediaFrameSourceGroup^ group)
{
   if (m_mediaCapture != nullptr)
   {
      // Already initialized.
      return task_from_result(true);
   }

   // Initialize mediacapture with the source group.
   m_mediaCapture = ref new MediaCapture();

   auto settings = ref new MediaCaptureInitializationSettings();

   // Select the source we will be reading from.
   settings->SourceGroup = group;

   // This media capture can share streaming with other apps.
   settings->SharingMode = MediaCaptureSharingMode::SharedReadOnly;

   // Only stream video and don't initialize audio capture devices.
   settings->StreamingCaptureMode = StreamingCaptureMode::Video;

   // Set to CPU to ensure frames always contain CPU SoftwareBitmap images,
   // instead of preferring GPU D3DSurface images.
   settings->MemoryPreference = MediaCaptureMemoryPreference::Cpu;

   // Only stream video and don't initialize audio capture devices.
   settings->StreamingCaptureMode = StreamingCaptureMode::Video;

   // Initialize MediaCapture with the specified group.
   // This must occur on the UI thread because some device families
   // (such as Xbox) will prompt the user to grant consent for the
   // app to access cameras.
   // This can raise an exception if the source no longer exists,
   // or if the source could not be initialized.
   return create_task(m_mediaCapture->InitializeAsync(settings))
      .then([this](task<void> initializeMediaCaptureTask)
   {
      try
      {
         // Get the result of the initialization. This call will throw if initialization failed
         // This pattern is docuemnted at https://msdn.microsoft.com/en-us/library/dd997692.aspx
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

task<void> DepthSensor::CleanupMediaCaptureAsync()
{
   task<void> cleanupTask = task_from_result();

   if (m_mediaCapture != nullptr)
   {
      m_frameReader->FrameArrived -= m_frameArrivedToken;
      cleanupTask = cleanupTask && create_task(m_frameReader->StopAsync());

      cleanupTask = cleanupTask.then([this] {
         OutputDebugString(L"Cleaning up MediaCapture...\n");
         m_mediaCapture = nullptr;
      });
   }
   return cleanupTask;
}


void DepthSensor::FrameReader_FrameArrived(MediaFrameReader^ sender, MediaFrameArrivedEventArgs^ args)
{
   if (sender == nullptr)
   {
      return;
   }

   if (MediaFrameReference^ frame = sender->TryAcquireLatestFrame())
   {
      if (frame != nullptr)
      {
         m_readingBitmap.lock();

         std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);

         // Find the corresponding source id
         assert(m_volatileState.m_FrameReadersToSourceIdMap.count(sender->GetHashCode()) != 0);
         int sourceId = m_volatileState.m_FrameReadersToSourceIdMap[sender->GetHashCode()];

         String^ name = m_volatileState.m_names[sourceId];

         if (name == ref new String(L"Short Throw ToF Depth"))
            //if (name == ref new String(L"Long Throw ToF Depth"))
         {
            m_latestBitmap = frame->VideoMediaFrame->SoftwareBitmap;
            m_hasNewBitmap = true;
         }

         m_readingBitmap.unlock();
      }
   }
}
