#include "pch.h"

#include "IO.h"

#include "Io\All.h"

using namespace HoloHands;
using namespace Concurrency;
using namespace Platform;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

SoftwareBitmap^ ConvertFromGray16ToRGBA8(SoftwareBitmap^ bitmap)
{
   SoftwareBitmap^ source = SoftwareBitmap::Copy(bitmap);

   Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer =
      source->LockBuffer(
         Windows::Graphics::Imaging::BitmapBufferAccessMode::ReadWrite);

   uint32_t pixelBufferByteCount = 0;
   uint16_t* pixelBufferData =
      Io::GetTypedPointerToMemoryBuffer<uint16_t>(bitmapBuffer->CreateReference(), pixelBufferByteCount);

   SoftwareBitmap^ dest = ref new SoftwareBitmap(BitmapPixelFormat::Bgra8, source->PixelWidth, source->PixelHeight);

   Windows::Graphics::Imaging::BitmapBuffer^ convBuffer =
      dest->LockBuffer(
         Windows::Graphics::Imaging::BitmapBufferAccessMode::ReadWrite);

   uint32_t convBufferByteCount = 0;
   uint8_t* convBufferData =
      Io::GetTypedPointerToMemoryBuffer<uint8_t>(convBuffer->CreateReference(), convBufferByteCount);

   for (size_t i = 0; i < convBufferByteCount; i += 4)
   {
      uint16_t value = pixelBufferData[i / 4];
      double max = 1000.0;
      uint8_t scaled = static_cast<uint8_t>((static_cast<double>(value) / max) * 255.0);

      if (value > max)
      {
         OutputDebugString(L"Values are being truncated");
      }

      convBufferData[i + 0] = scaled;
      convBufferData[i + 1] = scaled;
      convBufferData[i + 2] = scaled;
      convBufferData[i + 3] = 255;
   }

   return dest;
}

void IO::SaveToFile(SoftwareBitmap^ bitmap)
{
   auto rgbaBitmap = ConvertFromGray16ToRGBA8(bitmap);

   task<void> saveTask = SaveSoftwareBitmapAsync(rgbaBitmap);
}

task<void> IO::SaveSoftwareBitmapAsync(SoftwareBitmap^ bitmap)
{
   return create_task(StorageLibrary::GetLibraryAsync(KnownLibraryId::Pictures))
      .then([this, bitmap](StorageLibrary^ picturesLibrary)
   {
      auto captureFolder = picturesLibrary->SaveFolder;

      return create_task(captureFolder->CreateFileAsync("temp.jpg", CreationCollisionOption::ReplaceExisting))
         .then([bitmap](StorageFile^ file)
      {
         return create_task(file->OpenAsync(FileAccessMode::ReadWrite));
      }).then([this, bitmap](Streams::IRandomAccessStream^ outputStream)
      {
         return create_task(BitmapEncoder::CreateAsync(BitmapEncoder::JpegEncoderId, outputStream))
            .then([bitmap](BitmapEncoder^ encoder)
         {
            encoder->SetSoftwareBitmap(bitmap);

            return create_task(encoder->FlushAsync());
         }).then([this, outputStream](task<void> previousTask)
         {
            delete outputStream;
            previousTask.get();

            OutputDebugString(L"Bitmap saved!");
         });
      });
   });
}
