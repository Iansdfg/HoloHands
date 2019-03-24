#include "pch.h"

#include "MathsUtils.h"

using namespace HoloHands;

Eigen::Matrix4f MathsUtils::Convert(const Windows::Foundation::Numerics::float4x4& mat)
{
   Eigen::Matrix4f m = Eigen::Matrix4f::Identity();

   m(0, 0) = mat.m11;
   m(0, 1) = mat.m12;
   m(0, 2) = mat.m13;
   m(0, 3) = mat.m14;
   m(1, 0) = mat.m21;
   m(1, 1) = mat.m22;
   m(1, 2) = mat.m23;
   m(1, 3) = mat.m24;
   m(2, 0) = mat.m31;
   m(2, 1) = mat.m32;
   m(2, 2) = mat.m33;
   m(2, 3) = mat.m34;
   m(3, 0) = mat.m41;
   m(3, 1) = mat.m42;
   m(3, 2) = mat.m43;
   m(3, 3) = mat.m44;

   return m;
}