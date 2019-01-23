#pragma once

namespace HoloHands
{
   struct Defect
   {
      Defect()
         :
         Depth(0)
      {}

      cv::Point Start;
      cv::Point End;
      cv::Point Far;
      float Depth;
   };

   class HandDetector
   {
   public:
      void Process(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap, cv::Mat& outMat);

   private:
      const double MAX_IMAGE_DEPTH = 1000;
      const double MIN_CONTOUR_SIZE = 10;
      const double MIN_DEFECT_DEPTH = 20;

      void CalculateBounds(const std::vector<std::vector<cv::Point>>& contours, std::vector<cv::Rect>& bounds);

      bool Intersects(const cv::Rect& a, cv::Rect& b);

      void FilterContour(
         const std::vector<cv::Point>& newContour,
         const cv::Rect& newBounds,
         std::vector<std::vector<cv::Point>>& filteredContours,
         std::vector<cv::Rect>& filteredBounds);

      void FilterContours(
         const std::vector<std::vector<cv::Point>>& rawCountours,
         const std::vector<cv::Rect>& rawBounds,
         std::vector<std::vector<cv::Point>>& filteredContours,
         std::vector<cv::Rect>& filteredBounds);

      Defect ExtractDefect(const std::vector<cv::Point>& contour, const cv::Vec4i& defectIndices);
   };
}