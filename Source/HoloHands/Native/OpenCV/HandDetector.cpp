#include "pch.h"

#include "HandDetector.h"

#include "Converter.h"
#include "Io/All.h"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;
using namespace cv;

HoloHands::HandDetector::HandDetector()
   :
   m_isClosedHand(false)
{
}

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

   //Filter
   for (size_t i = 0; i < rawCountours.size(); i++)
   {
      FilterContour(rawCountours[i], rawBounds[i], filteredContours, filteredBounds);
   }
}

int HandDetector::FindLargestContour(
   const std::vector<std::vector<Point>>& contours,
   const std::vector<Rect>& bounds)
{
   int largest = 0;

   for (size_t i = 0; i < contours.size(); i++)
   {
      if (bounds[i].area() > bounds[largest].area())
      {
         largest = i;
      }
   }

   return largest;
}

void HandDetector::CalculateHandPosition(const std::vector<Point>& contour, Mat& mat, Point& position, Point& direction)
{
   std::vector<Point>hull;
   std::vector<int>hullIndices;

   convexHull(Mat(contour), hull);
   convexHull(Mat(contour), hullIndices, false, false);

   //Calculate defects for each hull.
   std::vector<Vec4i> defects;
   convexityDefects(contour, hullIndices, defects);
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

      circle(mat, largestDefect.Start, 2, Scalar(150), 2);
      circle(mat, largestDefect.End, 2, Scalar(150), 2);
      circle(mat, largestDefect.Far, 3, Scalar(150), 2);
      circle(mat, midPoint, 3, Scalar(255), 2);

      position = midPoint;
      
      //Calculate COM
      auto moment = cv::moments(contour);
      Point center(moment.m10 / moment.m00, moment.m01 / moment.m00);

      //Calculate direction to position.
      direction = midPoint - center;
      line(mat, midPoint, center, Scalar(100));
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

Mat HandDetector::ProcessOpenHand(const Mat& hands)
{

   //Get hand positions.
   Mat hullMat(hands.cols, hands.rows, CV_8UC1, Scalar(0));

   Point position;
   Point direction;
   CalculateHandPosition(m_contour, hullMat, position, direction);

   m_leftPosition = position; //TODO: largest should not == left.
   m_leftDirection = direction;

   //Draw hulls and contours.
   //for (int i = 0; i < largestContour.size(); i++)
   //{
      //drawContours(hullMat, largestContour, 0, Scalar(255));
      //drawContours(hullMat, hull, i, Scalar(255));
   //}

   return hullMat;
}

//TODO; support both hands.
Mat HandDetector::ProcessClosedHand(const Mat& hands)
{
   line(hands, m_leftPosition, m_leftPosition - m_leftDirection, Scalar(200));

   if (m_contour.size() > 0)
   {
     auto leftMostPoint = m_contour.front();
      for (auto& p : m_contour)
      {
         if (p.x < leftMostPoint.x)
         {
            leftMostPoint = p;
         }
      }

      circle(hands, leftMostPoint, 3, Scalar(255), 4);
   }

   return hands;
}

void HandDetector::Process(SoftwareBitmap^ input)
{
   Mat original;
   Converter::Convert(input, original); //convert to opencv mat.

   Mat scaled = original / MAX_IMAGE_DEPTH * 255.0; //scale to within 8bit range.

   scaled.convertTo(scaled, CV_8UC1); //make correct format of opencv.

   Mat mask;
   threshold(scaled, mask, 90, 255, CV_THRESH_BINARY);
   mask = 255 - mask; //invert

   Mat hands;
   scaled.copyTo(hands, mask);

   Mat cannyMat;
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

   //Check contours found.
   if (filteredContours.size() == 0)
   {
      m_image = hands;
      return;
   }

   //Find largest contour
   int largestIndex = FindLargestContour(filteredContours, filteredBounds);
   auto& largestContour = filteredContours[largestIndex];

   m_contour = largestContour;


   if (m_isClosedHand)
   {
      m_image = ProcessClosedHand(hands);
   }
   else
   {
      m_image = ProcessOpenHand(hands);
   }
}
