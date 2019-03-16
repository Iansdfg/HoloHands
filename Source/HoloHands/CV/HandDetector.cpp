#include "pch.h"

#include "HandDetector.h"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;
using namespace cv;

HoloHands::HandDetector::HandDetector()
   :
   _isClosedHand(false)
{
}

void HandDetector::Process(cv::Mat& input)
{
   _defectExtractor.SetImageSize(Size(input.size()));

   Mat scaled = input / MAX_IMAGE_DEPTH * 255.0; //Scale to within 8bit range.
   scaled.convertTo(scaled, CV_8UC1); //Make correct format for OpenCV.

   Mat mask;
   threshold(scaled, mask, 90, 255, CV_THRESH_BINARY);
   mask = 255 - mask; //Invert.

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

   _debugImage = hands;

   //Check contours found.
   if (filteredContours.size() == 0)
   {
      return;
   }

   //Find largest contour
   int largestIndex = FindLargestContour(filteredContours, filteredBounds);
   auto& largestContour = filteredContours[largestIndex];

   _contour = largestContour;

   if (_isClosedHand)
   {
      ProcessClosedHand();
   }
   else
   {
      ProcessOpenHand();
   }
}

void HoloHands::HandDetector::ShowDebugInfo(bool enabled)
{
   _showDebugInfo = enabled;
   _defectExtractor.ShowDebugInfo(enabled);
}

void HandDetector::CalculateBounds(const std::vector<std::vector<Point>>& contours, std::vector<Rect>& bounds)
{
   bounds.clear();
   for (auto& contour : contours)
   {
      bounds.push_back(boundingRect(contour));
   }
}

bool HandDetector::Intersects(const Rect& a, const Rect& b)
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

void HandDetector::CalculateHandPosition(const std::vector<Point>& contour, Point& position, Point& direction)
{
   Defect defect;
   if (_defectExtractor.FindDefect(contour, defect))
   {
      //Draw hull defects.
      Point midPoint = (defect.Start + defect.End) / 2.0;
      position = midPoint;
      
      //Calculate COM
      auto moment = cv::moments(contour);
      _handCenter = cv::Point(moment.m10 / moment.m00, moment.m01 / moment.m00);

      //Calculate direction to position.
      direction = midPoint - _handCenter;

      if (_showDebugInfo)
      {
         //Draw debug info.
         circle(_debugImage, defect.Start, 2, Scalar(150), 2);
         circle(_debugImage, defect.End, 2, Scalar(150), 2);
         circle(_debugImage, defect.Far, 3, Scalar(150), 2);
         circle(_debugImage, midPoint, 3, Scalar(255), 2);

         line(_debugImage, midPoint, _handCenter, Scalar(100));
      }
   }
}

void HandDetector::ProcessOpenHand()
{
   Point position;
   Point direction;
   CalculateHandPosition(_contour, position, direction);

   _handPosition = position;
   _leftDirection = direction;


   //if (_showDebugInfo)
   //{
   //   drawContours(_debugImage, _contour, 0, Scalar(255));
   //}
}

void HandDetector::ProcessClosedHand()
{
   if (_contour.size() > 0)
   {
     auto leftMostPoint = _contour.front();
      for (auto& p : _contour)
      {
         if (p.x < leftMostPoint.x)
         {
            leftMostPoint = p;
         }
      }

      if (_showDebugInfo)
      {
         line(_debugImage, _handPosition, _handPosition - _leftDirection, Scalar(200));
         circle(_debugImage, leftMostPoint, 3, Scalar(255), 4);
      }
   }
}
