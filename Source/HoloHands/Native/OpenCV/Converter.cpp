#include "pch.h"

#include "Converter.h"

#include "Io/BufferHelpers.h"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;

void Converter::Convert(SoftwareBitmap^ bitmap, cv::Mat& outMatrix)
{
   BitmapBuffer^ bitmapBuffer = bitmap->LockBuffer(BitmapBufferAccessMode::Read);

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

   //outMatrix.convertTo(outMatrix, CV_8U);
}