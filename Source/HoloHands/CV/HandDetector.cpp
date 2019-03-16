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

bool HandDetector::Process(cv::Mat& input)
{
   _imageSize = Size(input.size());
   _defectExtractor.SetImageSize(_imageSize);

   Mat scaled = input / MAX_IMAGE_DEPTH * 255.0; //Scale to within 8bit range.
   scaled.convertTo(scaled, CV_8UC1); //Make correct format for OpenCV.

   Mat mask;
   threshold(scaled, mask, MAX_DETECTION_THRESHOLD, 255, CV_THRESH_BINARY);
   mask = 255 - mask; //Invert.

   Mat hands;
   scaled.copyTo(hands, mask);

   Mat cannyMat;
   Canny(hands, cannyMat, 200, 250);

   blur(cannyMat, cannyMat, Size(6, 6));

   //Find contours.
   std::vector<std::vector<Point>> contours;
   findContours(cannyMat, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

   //Get rectangular bounds for all the contours.
   std::vector<Rect> bounds;
   CalculateBounds(contours, bounds);

   if (_showDebugInfo)
   {
      //Save debug image.
      _debugImage = hands;
   }

   //Select best contour.
   std::vector<Point> finalContour = FindBestContour(contours, bounds);

   //Check best contour.
   if (finalContour.size() == 0)
   {
      return false;
   }

   //Check best contour size.

   if (_isClosedHand)
   {
      ProcessClosedHand(finalContour);
   }
   else
   {
      ProcessOpenHand(finalContour);
   }

   return true;
}

void HandDetector::ShowDebugInfo(bool enabled)
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
      //Check if new contour overlaps with existing contours.
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

std::vector<Point> HandDetector::FindBestContour(
   const std::vector<std::vector<Point>>& rawCountours,
   const std::vector<Rect>& rawBounds)
{
   std::vector<std::vector<Point>> filteredContours;
   std::vector<Rect> filteredBounds;

   for (size_t i = 0; i < rawCountours.size(); i++)
   {
      //Remove small and merge overlapping contours.
      FilterContour(rawCountours[i], rawBounds[i], filteredContours, filteredBounds);
   }

   int contourCandidateIndex = -1;
   double contourCandiadateScore = 0;

   //Find contour with the highest score.
   for (size_t i = 0; i < filteredContours.size(); i++)
   {
      double score = CalculateContourScore(filteredContours[i], filteredBounds[i]);

      if (score > contourCandiadateScore)
      {
         contourCandiadateScore = score;
         contourCandidateIndex = i;
      }
   }

   if (_showDebugInfo)
   {
      if (contourCandidateIndex != -1)
      {
         for (size_t i = 0; i < filteredContours.size(); i++)
         {
            drawContours(_debugImage, filteredContours, i, Scalar(255));
            rectangle(_debugImage, filteredBounds[i], Scalar(100));
         }
         drawContours(_debugImage, filteredContours, contourCandidateIndex, Scalar(255), 3);
         rectangle(_debugImage, filteredBounds[contourCandidateIndex], Scalar(100), 3);
      }
   }

   if (contourCandidateIndex >= 0)
   {
      return filteredContours[contourCandidateIndex];
   }
   return {};

}

double HandDetector::CalculateContourScore(const std::vector<cv::Point>& countour, const cv::Rect& bound)
{
   Point imageCenter = Point(_imageSize.width / 2, _imageSize.height / 2);
   Point boundsCenter = (bound.br() + bound.tl()) * 0.5;

   double centrality = (_imageSize.width / 2) - cv::norm(imageCenter - boundsCenter);
   double area = bound.area();

   return
      centrality * CONTOUR_CENTRALITY_BIAS +
      area * CONTOUR_AREA_BIAS;
}

void HandDetector::ProcessOpenHand(const std::vector<Point>& contour)
{
   Point position;
   Point direction;

   Defect defect;
   if (_defectExtractor.FindDefect(contour, defect))
   {
      //Draw hull defects.
      Point midPoint = (defect.Start + defect.End) / 2.0;
      position = midPoint;

      //Calculate COM
      auto moment = cv::moments(contour);
      _handCenter = cv::Point(
         static_cast<int>(moment.m10 / moment.m00),
         static_cast<int>(moment.m01 / moment.m00));

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

   _handPosition = position;
   _leftDirection = direction;
}

void HandDetector::ProcessClosedHand(const std::vector<Point>& contour)
{
   if (contour.size() > 0)
   {
      auto leftMostPoint = contour.front();
      for (auto& p : contour)
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
