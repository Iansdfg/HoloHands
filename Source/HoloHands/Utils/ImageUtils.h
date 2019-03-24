#pragma once

namespace HoloHands
{
   class ImageUtils
   {
   public:
      static void Convert(Windows::Graphics::Imaging::SoftwareBitmap^ from, cv::Mat& to);
      static void Convert(const cv::Mat& from, Windows::Graphics::Imaging::SoftwareBitmap^ to);
   };
}