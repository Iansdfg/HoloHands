#pragma once

namespace HoloHands
{
   class MathsUtils
   {
   public:
      static Windows::Foundation::Numerics::float4x4 MathsUtils::Convert(const Eigen::Matrix3f& mat);
      static Eigen::Matrix3f MathsUtils::Convert(const Windows::Foundation::Numerics::float4x4& mat);
   };
}