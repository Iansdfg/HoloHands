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
      HandDetector();
      void Process(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);

      void IsClosed(bool isClosed) { m_isClosedHand = isClosed; }
      cv::Mat GetImage() { return m_image; }
      cv::Point GetLeftHandPosition() { return m_leftPosition; }
      cv::Point GetRightHandPosition() { return m_rightPosition; }

   private:
      const double MAX_IMAGE_DEPTH = 1000;
      const double MIN_CONTOUR_SIZE = 10;
      const double MIN_DEFECT_DEPTH = 20;

      bool m_isClosedHand;
      cv::Point m_leftPosition;
      cv::Point m_rightPosition;
      cv::Point m_leftDirection;
      cv::Point m_rightDirection;
      cv::Mat m_image;
      std::vector<cv::Point> m_contour;


      cv::Mat ProcessOpenHand(const cv::Mat& hands);
      cv::Mat ProcessClosedHand(const cv::Mat& hands);

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

      int FindLargestContour(
         const std::vector<std::vector<cv::Point>>& contours,
         const std::vector<cv::Rect>& bounds);

      void CalculateHandPosition(const std::vector<cv::Point>& contour, cv::Mat& mat, cv::Point& position, cv::Point& direction);
   };
}