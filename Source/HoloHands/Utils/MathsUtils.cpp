#include "pch.h"

#include "MathsUtils.h"

using namespace HoloHands;

Windows::Foundation::Numerics::float4x4 MathsUtils::Convert(const Eigen::Matrix3f& mat)
{
   auto m = Windows::Foundation::Numerics::float4x4::identity();

   m.m11 = mat(0, 0);
   m.m12 = mat(0, 1);
   m.m13 = mat(0, 2);
   m.m21 = mat(1, 0);
   m.m22 = mat(1, 1);
   m.m23 = mat(1, 2);
   m.m31 = mat(2, 0);
   m.m32 = mat(2, 1);
   m.m33 = mat(2, 2);

   return m;
}

Eigen::Matrix3f MathsUtils::Convert(const Windows::Foundation::Numerics::float4x4& mat)
{
   Eigen::Matrix3f m = Eigen::Matrix3f::Identity();

   m(0, 0) = mat.m11;
   m(0, 1) = mat.m12;
   m(0, 2) = mat.m13;
   m(1, 0) = mat.m21;
   m(1, 1) = mat.m22;
   m(1, 2) = mat.m23;
   m(2, 0) = mat.m31;
   m(2, 1) = mat.m32;
   m(2, 2) = mat.m33;

   return m;
}