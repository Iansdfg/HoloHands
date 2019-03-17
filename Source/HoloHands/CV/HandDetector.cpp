#include "pch.h"

#include "HandDetector.h"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;
using namespace cv;

HoloHands::HandDetector::HandDetector()
   :
   _isClosed(false)
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
   if (finalContour.size() == 0)
   {
      return false;
   }

   if (_isClosed)
   {
      ProcessClosedHand(finalContour);
   }
   else
   {
      ProcessOpenHand(finalContour);
   }

   _handDepth = CalculateDepth(input);

   if (_showDebugInfo)
   {
      putText(_debugImage, std::to_string(_handDepth), cvPoint(40, 40), CV_FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255));
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

std::vector<Point> HandDetector::FindBestContour(
   const std::vector<std::vector<Point>>& rawCountours,
   const std::vector<Rect>& rawBounds)
{
   std::vector<std::vector<Point>> filteredContours;
   std::vector<Rect> filteredBounds;

   for (size_t i = 0; i < rawCountours.size(); i++)
   {
      //Filter out small contours.
      if (rawBounds[i].width > MIN_CONTOUR_SIZE && rawBounds[i].height > MIN_CONTOUR_SIZE)
      {
         filteredContours.push_back(rawCountours[i]);
         filteredBounds.push_back(rawBounds[i]);
      }
   }

   int contourCandidateIndex = -1;
   float contourCandiadateScore = 0;

   //Find contour with the highest score.
   for (size_t i = 0; i < filteredContours.size(); i++)
   {
      float score = CalculateContourScore(filteredContours[i], filteredBounds[i]);

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

float HandDetector::CalculateContourScore(const std::vector<cv::Point>& countour, const cv::Rect& bound)
{
   Point2f imageCenter(_imageSize.width * 0.5f, _imageSize.height * 0.5f);
   Point2f boundsCenter = (bound.br() + bound.tl()) * 0.5f;

   float centrality = (_imageSize.width / 2.f) - static_cast<float>(cv::norm(imageCenter - boundsCenter));
   float area = static_cast<float>(bound.area());

   return
      centrality * CONTOUR_CENTRALITY_BIAS +
      area * CONTOUR_AREA_BIAS;
}

void HandDetector::ProcessOpenHand(const std::vector<Point>& contour)
{
   Point2f position;
   Point2f direction;

   Defect defect;
   if (_defectExtractor.FindDefect(contour, defect))
   {
      //Draw hull defects.
      Point2f midPoint = (defect.Start + defect.End) / 2.f;
      position = midPoint;

      //Calculate COM
      auto moment = moments(contour);
      Point2f handCenter(
         static_cast<float>(moment.m10 / moment.m00),
         static_cast<float>(moment.m01 / moment.m00));

      //Calculate direction to position.
      _direction = midPoint - handCenter;
      _direction /= norm(_direction); //Normalise.

      if (_showDebugInfo)
      {
         //Draw debug info.
         circle(_debugImage, defect.Start, 6, Scalar(255), 1);
         circle(_debugImage, defect.End, 6, Scalar(255), 1);
         circle(_debugImage, defect.Far, 6, Scalar(255), 1);

         float crossSize = 6.f;
         line(_debugImage, midPoint - Point2f(crossSize, 0), midPoint + Point2f(crossSize, 0), Scalar(255), 2);
         line(_debugImage, midPoint - Point2f(0, crossSize), midPoint + Point2f(0, crossSize), Scalar(255), 2);

         line(_debugImage, midPoint, handCenter, Scalar(200));
      }

      _handPosition = ApplySmoothing(position);
      _finger1Position = Point2f(defect.Start);
      _finger2Position = Point2f(defect.End);
      _palmPosition = Point2f(defect.Far);
   }
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
         line(_debugImage, _handPosition, Point2f(_handPosition) + _direction, Scalar(200));
         circle(_debugImage, leftMostPoint, 3, Scalar(255), 4);
      }
   }
}

float HandDetector::SampleDepthInDirection(
   const Mat& depthInput,
   const Point2f& startPoint,
   const Point2f& direction)
{
   float totalDepth = 0;
   int totalSampleCount = 0;

   for (int i = 0; i < DEPTH_SAMPLE_COUNT; i++)
   {
      Point point = startPoint + (direction * DEPTH_SAMPLE_OFFSET) + (direction * DEPTH_SAMPLE_SPACING * i);
      float sample = static_cast<float>(depthInput.at<unsigned short>(point));;
      if (sample > DEPTH_SAMPLE_MIN && sample < DEPTH_SAMPLE_MAX)
      {
         totalSampleCount++;
         totalDepth += sample;
      }
   }

   if (totalSampleCount > 0)
   {
      return totalDepth / totalSampleCount;
   }
   return 0;
}

Point2f HandDetector::ApplySmoothing(const Point2f& position)
{
   Point2f previous = _handPosition;
   Point2f total = previous * POSITION_SMOOTHING + position;
   
   return total / (1.f + POSITION_SMOOTHING);
}

float HandDetector::CalculateDepth(const cv::Mat& depthInput)
{
   float totalSamples =
      SampleDepthInDirection(depthInput, _finger1Position, -_direction) +
      SampleDepthInDirection(depthInput, _finger2Position, -_direction);

  return totalSamples / 2.0;
}