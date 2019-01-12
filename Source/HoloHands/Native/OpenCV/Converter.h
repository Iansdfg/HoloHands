#pragma once

namespace HoloHands
{
   class Converter
   {
   public:
      static void Convert(Windows::Graphics::Imaging::SoftwareBitmap^ from, cv::Mat& to);
   };
}