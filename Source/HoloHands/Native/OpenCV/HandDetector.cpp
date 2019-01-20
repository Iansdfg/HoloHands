#include "pch.h"

#include "HandDetector.h"

#include "Converter.h"
#include "Io/All.h"

#include "opencv2/ml.hpp"

using namespace HoloHands;
using namespace Windows::Graphics::Imaging;

void HandDetector::Process(SoftwareBitmap^ input, cv::Mat& output)
{
   cv::Mat depth;
   Converter::Convert(input, depth); //convert to opencv mat.

   double max = 1000.0;
   cv::Mat scaledDepth = depth / max * 255.0; //scale to within 8bit range.

   scaledDepth.convertTo(scaledDepth, CV_8UC1); //make correct format of opencv.

   cv::Mat mask;
   cv::threshold(scaledDepth, mask, 90, 255, CV_THRESH_BINARY); //limit to arbitrary depth, to remove noise.
   mask = 255 - mask; //invert mask.



   scaledDepth.copyTo(output, mask); //copy data where mask values are > 0.
}