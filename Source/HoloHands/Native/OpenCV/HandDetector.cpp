#include "pch.h"

#include "HandDetector.h"

#include "Converter.h"
#include "Io/All.h"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;
using namespace cv;

void HandDetector::CalculateBounds(const std::vector<std::vector<Point>>& contours, std::vector<Rect>& bounds)
{
   bounds.clear();
   for (auto& contour : contours)
   {
      bounds.push_back(boundingRect(contour));
   }
}

bool HandDetector::Intersects(const Rect& a, Rect& b)
{
   return (a & b).area() > 0;
}

void HandDetector::FilterContour(
   const std::vector<Point>& newContour,
   const Rect& newBounds,
   std::vector<std::vector<Point>>& filteredContours,
   std::vector<Rect>& filteredBounds)
{
   //Ignore small contours.
   if (newBounds.width > MIN_CONTOUR_SIZE && newBounds.height > MIN_CONTOUR_SIZE)
   {
      size_t overlappingIndex = -1;
      for (size_t i = 0; i < filteredContours.size(); i++)
      {
         if (Intersects(newBounds, filteredBounds[i]))
         {
            overlappingIndex = i;
            break;
         }
      }

      if (overlappingIndex == -1)
      {
         //Does not overlap.
         filteredContours.push_back(newContour);
         filteredBounds.push_back(newBounds);
      }
      else
      {
         //Merge overlapping contours.
         std::move(
            filteredContours[overlappingIndex].begin(),
            filteredContours[overlappingIndex].end(),
            std::back_inserter(const_cast<std::vector<Point>&>(newContour)));

         //Calculate new bounding rect.
         filteredBounds[overlappingIndex] = boundingRect(filteredContours[overlappingIndex]);
      }
   }
}

void HandDetector::FilterContours(
   const std::vector<std::vector<Point>>& rawCountours,
   const std::vector<Rect>& rawBounds,
   std::vector<std::vector<Point>>& filteredContours,
   std::vector<Rect>& filteredBounds)
{
   filteredContours.clear();
   filteredBounds.clear();

   for (size_t i = 0; i < rawCountours.size(); i++)
   {
      FilterContour(rawCountours[i], rawBounds[i], filteredContours, filteredBounds);
   }
}

Defect HandDetector::ExtractDefect(const std::vector<Point>& contour, const Vec4i& defectIndices)
{
   Defect defect;
   defect.Start = Point(contour[defectIndices[0]]);
   defect.End = Point(contour[defectIndices[1]]);
   defect.Far = Point(contour[defectIndices[2]]);
   defect.Depth = defectIndices[3] / 256.0f;

   return defect;
}

void HandDetector::Process(SoftwareBitmap^ input, Mat& output)
{
   Mat original;
   Converter::Convert(input, original); //convert to opencv mat.

   Mat scaled = original / MAX_IMAGE_DEPTH * 255.0; //scale to within 8bit range.

   scaled.convertTo(scaled, CV_8UC1); //make correct format of opencv.

   Mat mask;// (original.cols, original.rows, CV_8UC1, Scalar(0));
   threshold(scaled, mask, 90, 255, CV_THRESH_BINARY);
   mask = 255 - mask; //invert

   Mat hands;// (original.cols, original.rows, CV_8UC1, Scalar(0));
   scaled.copyTo(hands, mask);

   Mat cannyMat;// (original.cols, original.rows, CV_8UC1, Scalar(0));
   Canny(hands, cannyMat, 200, 250);

   blur(cannyMat, cannyMat, Size(6, 6));

   //Find contours.
   std::vector<std::vector<Point>> contours;
   findContours(cannyMat, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

   //Get rectangular bounds for contours.
   std::vector<Rect> bounds;
   CalculateBounds(contours, bounds);

   //Filter contours
   std::vector<std::vector<Point>> filteredContours;
   std::vector<Rect> filteredBounds;
   FilterContours(contours, bounds, filteredContours, filteredBounds);

   Mat hullMat(original.cols, original.rows, CV_8UC1, Scalar(0));
   std::vector<std::vector<Point>>hull(filteredContours.size());
   std::vector<std::vector<int>>hullIndices(filteredContours.size());

   //Calculate hulls.
   for (int i = 0; i < filteredContours.size(); i++)
   {
      auto& contour = filteredContours[i];

      convexHull(Mat(contour), hull[i]);
      convexHull(Mat(contour), hullIndices[i], false, false);

      //Calculate defects for each hull.
      std::vector<Vec4i> defects;
      convexityDefects(contour, hullIndices[i], defects);
      Defect largestDefect;

      for (size_t d = 1; d < defects.size(); d++) //TODO: Ignoring the first defect, as it is incorrect. Why?
      {
         Defect defect = ExtractDefect(contour, defects[d]);

         if (defect.Depth > largestDefect.Depth)
         {
            largestDefect = defect;
         }
      }

      if (largestDefect.Depth > MIN_DEFECT_DEPTH)
      {
         //Draw hull defects.
         Point midPoint = (largestDefect.Start + largestDefect.End) / 2.0;
         circle(hullMat, largestDefect.Start, 2, Scalar(150), 2);
         circle(hullMat, largestDefect.End, 2, Scalar(150), 2);
         circle(hullMat, largestDefect.Far, 3, Scalar(150), 2);
         circle(hullMat, midPoint, 3, Scalar(255), 2);
      }
   }

   //Draw hulls and contours.
   for (int i = 0; i < filteredContours.size(); i++)
   {
      drawContours(hullMat, filteredContours, i, Scalar(255));
      drawContours(hullMat, hull, i, Scalar(255));
   }

   output = hullMat;
}