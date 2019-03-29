#pragma once

namespace HoloHands
{
   class ImageUtils
   {
   public:
      // Converts a SoftwareBitmap to OpenCV Mat.
      static void Convert(Windows::Graphics::Imaging::SoftwareBitmap^ from, cv::Mat& to);

      // Converts an OpenCV Mat to SoftwareBitmap.
      static void Convert(const cv::Mat& from, Windows::Graphics::Imaging::SoftwareBitmap^ to);
   };
}