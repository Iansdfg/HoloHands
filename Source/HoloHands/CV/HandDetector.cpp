#include "pch.h"

#include "HandDetector.h"

#include "Defect.h"

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

   //Create background mask.
   Mat mask;
   threshold(scaled, mask, MAX_DETECTION_THRESHOLD, 255, CV_THRESH_BINARY);
   mask = 255 - mask; //Invert.

   //Discard background information.
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
      _debugImage = scaled;
   }

   //Select best contour.
   std::vector<Point> finalContour = FindBestContour(contours, bounds);
   if (finalContour.size() == 0)
   {
      return false;
   }

   //Calculate 2d hand position.
   if (_isClosed)
   {
      ProcessClosedHand(finalContour);
   }
   else
   {
      ProcessOpenHand(finalContour);
   }

   //Calculate depth.
   _handDepth = CalculateDepth(input);

   if (_showDebugInfo)
   {
      //Draw hand position.
      float crossSize = 6.f;
      line(_debugImage, _handPosition - Point2f(crossSize, 0), _handPosition + Point2f(crossSize, 0), Scalar(255), 2);
      line(_debugImage, _handPosition - Point2f(0, crossSize), _handPosition + Point2f(0, crossSize), Scalar(255), 2);

      //Draw hand direction.
      line(_debugImage, _handPosition, _handPosition + _direction * 50, Scalar(200));

      if (_isClosed)
      {
         putText(_debugImage, "Closed", Point(20, 20), CV_FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255));
      }
      else
      {
         putText(_debugImage, "Open", Point(20, 20), CV_FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255));

         circle(_debugImage, _finger1Position, 6, Scalar(255), 1);
         circle(_debugImage, _finger2Position, 6, Scalar(255), 1);
         circle(_debugImage, _palmPosition, 6, Scalar(255), 1);
      }

      //Draw depth text.
      putText(_debugImage, std::to_string(_handDepth), Point(20, 40), CV_FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255));
   }

   return true;
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

      //Calculate direction.
      Point across = defect.Start - defect.End;
      _direction = Point(across.y, -across.x); //Orthogonal.
      _direction /= norm(_direction); //Normalise.

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
      int furthestIndex = 0;
      float furthestDistance = -FLT_MAX;
      for (int i = 0; i < static_cast<int>(contour.size()); i++)
      {
         float distance =  _direction.dot(contour[i]);
         if (distance > furthestDistance)
         {
            furthestIndex = i;
            furthestDistance = distance;
         }
      }

      _handPosition = contour[furthestIndex];
   }
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
      //Draw debug info.
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

float HandDetector::SampleDepthInDirection(
   const Mat& depthInput,
   const Point2f& startPoint,
   const Point2f& direction)
{
   float totalDepth = 0;
   int totalSampleCount = 0;

   //Iterate through all the samples.
   for (int i = 0; i < DEPTH_SAMPLE_COUNT; i++)
   {
      Point samplePosition = startPoint +
         (direction * DEPTH_SAMPLE_OFFSET) +
         (direction * DEPTH_SAMPLE_SPACING * i);

      float sampleDepth = static_cast<float>(depthInput.at<unsigned short>(samplePosition));

      if (sampleDepth > DEPTH_SAMPLE_MIN && sampleDepth < DEPTH_SAMPLE_MAX)
      {
         //Only use depths within a valid range.
         totalSampleCount++;
         totalDepth += sampleDepth;
      }
   }

   if (totalSampleCount > 0)
   {
      //Calculate the average of the samples.
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
   if (_isClosed)
   {
      //Calculate depth at hand position.
      return SampleDepthInDirection(depthInput, _handPosition, -_direction);
   }
   else
   {
      //Calculate average depth of both finger tips.
      float totalSamples =
         SampleDepthInDirection(depthInput, _finger1Position, -_direction) +
         SampleDepthInDirection(depthInput, _finger2Position, -_direction);

      return totalSamples / 2.0f;
   }
}