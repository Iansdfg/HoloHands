#pragma once

#include <opencv2/imgproc/imgproc.hpp>

namespace HoloHands
{
   //Structure to contain OpenCV defect data.
   struct Defect
   {
      Defect()
         :
         Depth(0)
      {}

      cv::Point Start;
      cv::Point End;
      cv::Point Far;
      cv::Point Mid;
      float Depth;
   };
}