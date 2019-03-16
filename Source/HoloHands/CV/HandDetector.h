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
      float GetHandDepth() { return _handDepth; }
      void ShowDebugInfo(bool enabled);

   private:
      const double MAX_IMAGE_DEPTH = 1000; //Scales the image to fit within this range.
      const double MAX_DETECTION_THRESHOLD = 120; //Higher == detects objects further away.
      const double MIN_CONTOUR_SIZE = 20; //Minimum size of a valid contour.
      const double CONTOUR_CENTRALITY_BIAS = 0.0; //Higher == More central contours will be selected.
      const double CONTOUR_AREA_BIAS = 1.0; //Higher == Larger contours will be selected.
      const double DEPTH_SAMPLE_LENGTH = 10.0; //Higher == Move the depth sample point deeper into the palm.
      const double DEPTH_SAMPLE_COUNT = 5.0; //Higher == Move depth samples per sample length.
      const double DEPTH_SAMPLE_OFFSET = 2.0;
      const double DEPTH_SAMPLE_SPACING = DEPTH_SAMPLE_LENGTH / DEPTH_SAMPLE_COUNT;
      const double MIN_DEPTH_SAMPLE = 200;
      const double MAX_DEPTH_SAMPLE = 1000;

      ConvexityDefectExtractor _defectExtractor;
      bool _isClosedHand;
      cv::Point _handPosition;
      cv::Point2d _finger1Position;
      cv::Point2d _finger2Position;
      cv::Point2d _palmPosition;
      cv::Point2d _direction;
      double _handDepth;
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

      double HandDetector::CalculateDepth(const cv::Mat& depthInput);

      double SampleDepthInDirection(
         const cv::Mat& depthInput,
         const cv::Point2d& startPoint,
         const cv::Point2d& direction);
   };
}