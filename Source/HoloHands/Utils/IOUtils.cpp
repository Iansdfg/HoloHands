#include "pch.h"

#include "IOUtils.h"

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
using namespace Concurrency;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Input::Spatial;
using namespace Microsoft::WRL;

void IOUtils::SaveToFile(SoftwareBitmap^ bitmap)
{
   auto rgbaBitmap = ConvertFromGray16ToBGRA8(bitmap);

   task<void> saveTask = SaveSoftwareBitmapAsync(rgbaBitmap, "holohands.jpg");
}

SoftwareBitmap^ IOUtils::ConvertFromGray16ToBGRA8(SoftwareBitmap^ bitmap)
{
   //To prevent to bitmap from being manipulated during copy.
   SoftwareBitmap^ source = SoftwareBitmap::Copy(bitmap);

   Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer =
      source->LockBuffer(
         Windows::Graphics::Imaging::BitmapBufferAccessMode::ReadWrite);

   uint32_t pixelBufferByteCount = 0;
   uint16_t* pixelBufferData = Io::GetTypedPointerToMemoryBuffer<uint16_t>(
      bitmapBuffer->CreateReference(),
      pixelBufferByteCount);

   //New bgra bitmap.
   SoftwareBitmap^ destination = ref new SoftwareBitmap(
      BitmapPixelFormat::Bgra8,
      source->PixelWidth,
      source->PixelHeight);

   Windows::Graphics::Imaging::BitmapBuffer^ convBuffer =
      destination->LockBuffer(
         Windows::Graphics::Imaging::BitmapBufferAccessMode::ReadWrite);

   uint32_t newBufferByteCount = 0;
   uint8_t* newBufferData =
      Io::GetTypedPointerToMemoryBuffer<uint8_t>(convBuffer->CreateReference(), newBufferByteCount);

   //Iterate through buffer to copy data.
   for (size_t i = 0; i < newBufferByteCount; i += 4)
   {
      uint16_t value = pixelBufferData[i / 4];
      double max = 65535.0;

      //Warning, loss of precision.
      uint8_t scaled = static_cast<uint8_t>((static_cast<double>(value) / max) * 255.0);

      if (value > max)
      {
         //This should not happen.
         OutputDebugString(L"Values are being truncated");
      }

      //Copy data.
      newBufferData[i + 0] = scaled;
      newBufferData[i + 1] = scaled;
      newBufferData[i + 2] = scaled;
      newBufferData[i + 3] = 255;
   }

   return destination;
}

task<void> IOUtils::SaveSoftwareBitmapAsync(SoftwareBitmap^ bitmap, String^ fileName)
{
   return create_task(StorageLibrary::GetLibraryAsync(KnownLibraryId::Pictures))
      .then([this, bitmap, fileName](StorageLibrary^ picturesLibrary)
   {
      auto captureFolder = picturesLibrary->SaveFolder;

      return create_task(captureFolder->CreateFileAsync(fileName, CreationCollisionOption::GenerateUniqueName))
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

std::vector<uint8_t> IOUtils::LoadBGRAImage(const wchar_t* filename, uint32_t& width, uint32_t& height)
{
   ComPtr<IWICImagingFactory> wicFactory;
   ASSERT_SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)));

   ComPtr<IWICBitmapDecoder> decoder;
   ASSERT_SUCCEEDED(wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf()));

   ComPtr<IWICBitmapFrameDecode> frame;
   ASSERT_SUCCEEDED(decoder->GetFrame(0, frame.GetAddressOf()));

   ASSERT_SUCCEEDED(frame->GetSize(&width, &height));

   WICPixelFormatGUID pixelFormat;
   ASSERT_SUCCEEDED(frame->GetPixelFormat(&pixelFormat));

   uint32_t rowPitch = width * sizeof(uint32_t);
   uint32_t imageSize = rowPitch * height;

   std::vector<uint8_t> image;
   image.resize(size_t(imageSize));

   if (memcmp(&pixelFormat, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID)) == 0)
   {
      ASSERT_SUCCEEDED(frame->CopyPixels(nullptr, rowPitch, imageSize, reinterpret_cast<BYTE*>(image.data())));
   }
   else
   {
      ComPtr<IWICFormatConverter> formatConverter;
      ASSERT_SUCCEEDED(wicFactory->CreateFormatConverter(formatConverter.GetAddressOf()));

      BOOL canConvert = FALSE;
      ASSERT_SUCCEEDED(formatConverter->CanConvert(pixelFormat, GUID_WICPixelFormat32bppBGRA, &canConvert));
      if (!canConvert)
      {
         throw std::exception("CanConvert");
      }

      ASSERT_SUCCEEDED(formatConverter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
         WICBitmapDitherTypeErrorDiffusion, nullptr, 0, WICBitmapPaletteTypeMedianCut));

      ASSERT_SUCCEEDED(formatConverter->CopyPixels(nullptr, rowPitch, imageSize, reinterpret_cast<BYTE*>(image.data())));
   }

   return image;
}
