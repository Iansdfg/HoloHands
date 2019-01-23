#include "pch.h"

#include "HandDetector.h"

#include "Converter.h"
#include "Io/All.h"

#include "opencv2/ml.hpp"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;
using namespace cv;

bool BoundingRectangleIntersects(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
   auto rectA = boundingRect(a);
   auto rectB = boundingRect(b);

   return ((rectA & rectB).area() > 0);
}

void FilterContour(std::vector<cv::Point>& newContour, std::vector<std::vector<cv::Point>>& filteredContours)
{
   cv::Point2f center;
   float radius;
   minEnclosingCircle(newContour, center, radius);

   //Ignore small contours.
   if (radius > 10)
   {
      int overlappingIndex = -1;
      for (int i = 0; i < filteredContours.size(); i++)
      {
         if (BoundingRectangleIntersects(newContour, filteredContours[i]))
         {
            overlappingIndex = i;
            break;
         }
      }

      if (overlappingIndex == -1)
      {
         filteredContours.push_back(newContour);
      }
      else
      {
         std::move(
            filteredContours[overlappingIndex].begin(),
            filteredContours[overlappingIndex].end(),
            std::back_inserter(newContour));
      }
   }
}

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

Defect ExtractDefect(const std::vector<cv::Point>& contour, const cv::Vec4i& defectIndices)
{
   Defect defect;
   defect.Start = cv::Point(contour[defectIndices[0]]);
   defect.End = cv::Point(contour[defectIndices[1]]);
   defect.Far = cv::Point(contour[defectIndices[2]]);
   defect.Depth = defectIndices[3] / 256.0f;

   return defect;
}

void HandDetector::Process(SoftwareBitmap^ input, cv::Mat& output)
{
   Mat original;
   Converter::Convert(input, original); //convert to opencv mat.

   double max = 1000.0;
   cv::Mat scaled = original / max * 255.0; //scale to within 8bit range.

   scaled.convertTo(scaled, CV_8UC1); //make correct format of opencv.

   Mat mask;// (original.cols, original.rows, CV_8UC1, Scalar(0));
   threshold(scaled, mask, 90, 255, CV_THRESH_BINARY);
   mask = 255 - mask; //invert

   Mat hands;// (original.cols, original.rows, CV_8UC1, Scalar(0));
   scaled.copyTo(hands, mask);

   Mat cannyMat;// (original.cols, original.rows, CV_8UC1, Scalar(0));
   Canny(hands, cannyMat, 200, 250);

   blur(cannyMat, cannyMat, cv::Size(6, 6));

   //Find contours.
   std::vector<std::vector<cv::Point>> contours;
   findContours(cannyMat, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

   //Filter contours
   std::vector<std::vector<cv::Point>> filteredContours;
   for (auto contour : contours)
   {
      FilterContour(contour, filteredContours);
   }

   Mat hullMat(original.cols, original.rows, CV_8UC1, Scalar(0));

   std::vector<std::vector<cv::Point>>hull(filteredContours.size());
   std::vector<std::vector<int>>hullIndices(filteredContours.size());

   for (int i = 0; i < filteredContours.size(); i++)
   {
      auto& contour = filteredContours[i];

      convexHull(Mat(contour), hull[i]);
      convexHull(Mat(contour), hullIndices[i], false, false);

      std::vector<Vec4i> defects;
      convexityDefects(contour, hullIndices[i], defects);
      Defect largestDefect;

      for (size_t d = 1; d < defects.size(); d++) //Ignoring the first defect, as it happens to be wrong. Does this work?
      {
         Defect defect = ExtractDefect(contour, defects[d]);

         if (defect.Depth > largestDefect.Depth)
         {
            largestDefect = defect;
         }
      }

      cv::Point midPoint = (largestDefect.Start + largestDefect.End) / 2.0;
      cv::circle(hullMat, largestDefect.Start, 2, Scalar(150), 2);
      cv::circle(hullMat, largestDefect.End, 2, Scalar(150), 2);
      cv::circle(hullMat, largestDefect.Far, 3, Scalar(150), 2);
      cv::circle(hullMat, midPoint, 3, Scalar(255), 2);
   }

   for (int i = 0; i < filteredContours.size(); i++)
   {
      drawContours(hullMat, filteredContours, i, Scalar(255));
      drawContours(hullMat, hull, i, Scalar(255));
   }

   output = hullMat;
}