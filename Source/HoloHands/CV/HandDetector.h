#pragma once

#include "CV/ConvexityDefectExtractor.h"

namespace HoloHands
{
   class HandDetector
   {
   public:
      HandDetector();
      bool Process(cv::Mat& input);

      void IsClosed(bool isClosed) { _isClosedHand = isClosed; }
      cv::Mat& GetDebugImage() { return _debugImage; }
      cv::Point GetHandPosition() { return _handPosition; }
      cv::Point GetHandCenter() { return _handCenter; }
      void ShowDebugInfo(bool enabled);

   private:
      const double MAX_IMAGE_DEPTH = 1000;
      const double MAX_DETECTION_THRESHOLD = 120; //Higher == detects objects further away.
      const double MIN_CONTOUR_SIZE = 20;
      const double CONTOUR_CENTRALITY_BIAS = 0.0;
      const double CONTOUR_AREA_BIAS = 1.0;

      ConvexityDefectExtractor _defectExtractor;
      bool _isClosedHand;
      cv::Point _handPosition;
      cv::Point _leftDirection;
      cv::Point _handCenter;
      cv::Mat _debugImage;
      bool _showDebugInfo;
      cv::Size _imageSize;

      void ProcessOpenHand(const std::vector<cv::Point>& contour);
      void ProcessClosedHand(const std::vector<cv::Point>& contour);

      static void CalculateBounds(
         const std::vector<std::vector<cv::Point>>& contours,
         std::vector<cv::Rect>& bounds);

      static bool Intersects(
         const cv::Rect& a,
         const cv::Rect& b);

      void FilterContour(
         const std::vector<cv::Point>& newContour,
         const cv::Rect& newBounds,
         std::vector<std::vector<cv::Point>>& filteredContours,
         std::vector<cv::Rect>& filteredBounds);

      std::vector<cv::Point> FindBestContour(
         const std::vector<std::vector<cv::Point>>& rawCountours,
         const std::vector<cv::Rect>& rawBounds);

      double CalculateContourScore(
         const std::vector<cv::Point>& countour,
         const cv::Rect& bound);
   };
}