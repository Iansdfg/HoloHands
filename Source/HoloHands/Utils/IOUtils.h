#pragma once

namespace HoloHands
{
   class IOUtils
   {
   public:
      // Saves a bitmap image to disk.
      // This is useful for debugging any bitmap being received from the sensor.
      // Currently only supports the image format of Gray 16 bit.
      void SaveToFile(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);

      // Loads the byte data from an image in the files system.
      std::vector<uint8_t> LoadBGRAImage(const wchar_t* filename, uint32_t& width, uint32_t& height);

   private:
      // Converts an Gray 16bit bitmap to BGRA 8 bit. There may be a loss of
      // precision when converting in to the smaller value range.
      Windows::Graphics::Imaging::SoftwareBitmap^ IOUtils::ConvertFromGray16ToBGRA8(
         Windows::Graphics::Imaging::SoftwareBitmap^ bitmap);

      // Asynchronously saves a software bitmap to file.
      concurrency::task<void> SaveSoftwareBitmapAsync(
         Windows::Graphics::Imaging::SoftwareBitmap^ bitmap,
         Platform::String^ fileName);
   };
}
