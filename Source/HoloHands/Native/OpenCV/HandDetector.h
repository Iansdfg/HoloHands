#pragma once

namespace HoloHands
{
   class HandDetector
   {
   public:
      void Process(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap, cv::Mat& outMat);
   };
}