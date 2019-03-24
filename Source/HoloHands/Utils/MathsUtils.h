#pragma once

namespace HoloHands
{
   class MathsUtils
   {
   public:
      static Eigen::Matrix4f MathsUtils::Convert(const Windows::Foundation::Numerics::float4x4& mat);
   };
}