#pragma once

namespace HoloHands
{
   class IO
   {
   public:
      void SaveToFile(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);

   private:
      concurrency::task<void> SaveSoftwareBitmapAsync(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);
   };
}
