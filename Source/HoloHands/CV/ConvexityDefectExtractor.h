#pragma once

namespace HoloHands
{
   struct Defect;

   class ConvexityDefectExtractor
   {
   public:
      ConvexityDefectExtractor();

      // Finds the most suitable defect for a given set of contours.
      bool FindDefect(
         const std::vector<cv::Point>& contour,
         Defect& outDefect);

      void ShowDebugInfo(bool enabled);
      void SetImageSize(const cv::Size& size);

   private:
      const double MIN_DEFECT_DEPTH = 20; //Minimum depth for a valid defect.
      const double HEIGHT_BIAS = 1.0; //Higher == Defects towards to top of the image will be selected.
      const double DEPTH_BIAS = 0.5; //Higher == Deeper defects will be selected.
      const double VERTICALITY_BIAS = 10.0; //Higher == Defects facing upwards will be selected.

      cv::Size _imageSize;
      bool _showDebugInfo;

      // Calculates a score for a given defect.
      // The higher to score, to more suitable the defect.
      double CalculateDefectScore(const Defect& defect);

      // Extract defect information from contours.
      static Defect GetDefectFromContour(
         const std::vector<cv::Point>& contour,
         const cv::Vec4i& defectIndices);
   };
}