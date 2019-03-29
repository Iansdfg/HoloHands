#include "pch.h"

#include "ImageUtils.h"

#include "Io/BufferHelpers.h"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;

void ImageUtils::Convert(SoftwareBitmap^ bitmap, cv::Mat& outMatrix)
{
   BitmapBuffer^ bitmapBuffer =
      bitmap->LockBuffer(BitmapBufferAccessMode::Read);

   uint32_t pixelBufferDataLength = 0;

   uint8_t* pixelBufferData =
      Io::GetTypedPointerToMemoryBuffer<uint8_t>(
         bitmapBuffer->CreateReference(),
         pixelBufferDataLength);

   auto format  = bitmap->BitmapPixelFormat;
   outMatrix = cv::Mat(
      bitmap->PixelHeight,
      bitmap->PixelWidth,
      CV_16UC1,
      pixelBufferData);
}

void ImageUtils::Convert(const cv::Mat& matrix, SoftwareBitmap^ outBitmap)
{
   BitmapBuffer^ bitmapBuffer =
      outBitmap->LockBuffer(BitmapBufferAccessMode::ReadWrite);

   uint32_t byteCount = 0;
   uint16_t* pixelBufferData = Io::GetTypedPointerToMemoryBuffer<uint16_t>(
      bitmapBuffer->CreateReference(),
      byteCount);

   int pixelCount = byteCount / 2;

   for (int i = 0; i < pixelCount; i++)
   {
      //Normalize uint8 to uint16 range.
      pixelBufferData[i] = (matrix.data[i] / UCHAR_MAX) * USHORT_MAX;
   }
}