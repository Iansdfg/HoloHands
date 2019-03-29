#pragma once

namespace HoloHands
{
   class MathsUtils
   {
   public:
      // Converts Windows Foundation matrix into an Eigen matrix.
      static Eigen::Matrix4f MathsUtils::Convert(const Windows::Foundation::Numerics::float4x4& mat);
   };
}