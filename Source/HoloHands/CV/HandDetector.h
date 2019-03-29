#pragma once

#include "CV/ConvexityDefectExtractor.h"

namespace HoloHands
{
   class HandDetector
   {
   public:
      HandDetector();

      // Calculate a 2D position from a given image using OpenCV.
      bool Process(cv::Mat& input);

      cv::Point2f GetHandPosition2D() { return _handPosition; }
      float GetHandDepth() { return _handDepth; }
      void SetIsClosed(bool isClosed) { _isClosed = isClosed; }
      void ShowDebugInfo(bool enabled);
      cv::Mat& GetDebugImage() { return _debugImage; }

   private:
      const float MAX_IMAGE_DEPTH = 1000; //Scales the image to fit within this range.
      const float MAX_DETECTION_THRESHOLD = 170; //Higher == detects objects further away.
      const float MIN_CONTOUR_SIZE = 40; //Minimum size of a valid contour.
      const float CONTOUR_CENTRALITY_BIAS = 0.0; //Higher == More central contours will be selected.
      const float CONTOUR_AREA_BIAS = 1.f; //Higher == Larger contours will be selected.
      const float DEPTH_SAMPLE_LENGTH = 5.f; //Higher == Move the depth sample point deeper into the palm.
      const float DEPTH_SAMPLE_COUNT = 3.f; //Higher == Move depth samples per sample length.
      const float DEPTH_SAMPLE_SPACING = DEPTH_SAMPLE_LENGTH / DEPTH_SAMPLE_COUNT;
      const float DEPTH_SAMPLE_OFFSET = 2.f; //Starting sampling offset in the sampling direction.
      const float DEPTH_SAMPLE_MIN = 200; //Minimum valid sample value, lower value will be discarded.
      const float DEPTH_SAMPLE_MAX = 1000; //Maximum valid sample value, higher value will be discarded.
      const float POSITION_SMOOTHING = 0.0f; //Higher == Positions are smoothed more with previous positions.

      ConvexityDefectExtractor _defectExtractor;
      bool _isClosed;
      cv::Point2f _handPosition;
      cv::Point2f _finger1Position;
      cv::Point2f _finger2Position;
      cv::Point2f _palmPosition;
      cv::Point2f _direction;
      float _handDepth;
      cv::Mat _debugImage;
      bool _showDebugInfo;
      cv::Size _imageSize;

      // Selects the mid point between the thumb and finger.
      void ProcessOpenHand(const std::vector<cv::Point>& contour);

      // Selects the furthest contour point in the hand's direction.
      // The hand direction is defined by the most recent open pose.
      void ProcessClosedHand(const std::vector<cv::Point>& contour);

      // Calculates a vector of bounds for a given vector of contours.
      static void CalculateBounds(
         const std::vector<std::vector<cv::Point>>& contours,
         std::vector<cv::Rect>& bounds);

      // Returns true when the two rectangles intersect.
      static bool Intersects(
         const cv::Rect& a,
         const cv::Rect& b);

      // Find the most suitable contours.
      std::vector<cv::Point> FindBestContour(
         const std::vector<std::vector<cv::Point>>& countours,
         const std::vector<cv::Rect>& bounds);

      // Calculates a score for a given contour.
      // The higher to score, to more suitable the contour.
      float CalculateContourScore(
         const std::vector<cv::Point>& countour,
         const cv::Rect& bound);

      // Calculate a depth at the current hand postion.
      float HandDetector::CalculateDepth(const cv::Mat& depthInput);

      // Samples depth value at multiples points in a given direction.
      float SampleDepthInDirection(
         const cv::Mat& depthInput,
         const cv::Point2f& startPoint,
         const cv::Point2f& direction);

      // Apply temporal smoothing the 2D position values.
      cv::Point2f HandDetector::ApplySmoothing(
         const cv::Point2f& position);
   };
}  