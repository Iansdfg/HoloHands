#include "pch.h"

#include "ConvexityDefectExtractor.h"

using namespace HoloHands;
using namespace cv;


ConvexityDefectExtractor::ConvexityDefectExtractor()
   :
   _showDebugInfo(false)
{
}

bool ConvexityDefectExtractor::FindDefect(
   const std::vector<Point>& contour,
   Defect& outDefect)
{
   std::vector<Point>hull;
   std::vector<int>hullIndices;

   convexHull(Mat(contour), hull);
   convexHull(Mat(contour), hullIndices, false, false);

   //Calculate defects for each hull.
   std::vector<Vec4i> defects;
   convexityDefects(contour, hullIndices, defects);

   Defect defectCandidate;
   double highestScore = 0;

   for (size_t d = 1; d < defects.size(); d++) //Ignore the first defect.
   {
      Defect defect = GetDefectFromContour(contour, defects[d]);
      if (defect.Depth > MIN_DEFECT_DEPTH)
      {
         double score = CalculateDefectScore(defect);

         if (score > highestScore)
         {
            highestScore = score;
            defectCandidate = defect;
         }
      }
   }

   if (highestScore > 0)
   {
      outDefect = defectCandidate;
      return true;
   }

   return false;
}

void ConvexityDefectExtractor::ShowDebugInfo(bool enabled)
{
   _showDebugInfo = enabled;
}

void ConvexityDefectExtractor::SetImageSize(const cv::Size& size)
{
   _imageSize = size;
}

double ConvexityDefectExtractor::CalculateDefectScore(const Defect& defect)
{
   const Point2d vertical(0, 1);
   Point2d position = defect.Far;
   Point2d direction = (defect.Mid - defect.Far);
   direction /= cv::norm(direction); //Normalise.

   double height = _imageSize.height - position.y;
   double depth = defect.Depth;
   double verticality = vertical.dot(direction);

   return
      height * HEIGHT_BIAS +
      depth * DEPTH_BIAS +
      verticality * VERTICALITY_BIAS;
}

Defect ConvexityDefectExtractor::GetDefectFromContour(
   const std::vector<Point>& contour,
   const Vec4i& defectIndices)
{
   Defect defect;
   defect.Start = Point(contour[defectIndices[0]]);
   defect.End = Point(contour[defectIndices[1]]);
   defect.Far = Point(contour[defectIndices[2]]);
   defect.Mid = (defect.Start + defect.End) / 2.0;
   defect.Depth = defectIndices[3] / 256.0f;

   return defect;
}
