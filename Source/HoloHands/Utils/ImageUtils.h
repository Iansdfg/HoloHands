#pragma once

namespace HoloHands
{
   class ImageUtils
   {
   public:
      static void Convert(Windows::Graphics::Imaging::SoftwareBitmap^ from, cv::Mat& to);
   };
}