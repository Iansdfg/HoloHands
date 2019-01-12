#include "pch.h"

#include "HandDetector.h"

#include "Converter.h"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;

void HandDetector::Process(SoftwareBitmap^ bitmap, cv::Mat& outMat)
{
   //cv::Mat depth;
   Converter::Convert(bitmap, outMat);

   //cv::Canny(depth, outMat, 0, 1);

   //TODO: outline hands properly.
}