/*
 * TestImgScanner.cpp
 *
 *  Created on: Jan 9/2016
 *      Author: nelson
 */

#include "imgscanner/ImgScannerSane.h"
#include "Image.h"
#include "DmScanLib.h"
#include "test/TestCommon.h"

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <memory>

using namespace dmscanlib;

namespace {

int IMAGE_PIXELS_THRESHOLD = 5;

std::unique_ptr<imgscanner::ImgScannerSane> selectSourceAsDefault() {
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner(new imgscanner::ImgScannerSane());

#ifdef WIN32
   imgScanner->selectSourceAsDefault();
#else
   std::vector<std::string> deviceNames;
   imgScanner->getDeviceNames(deviceNames);
   CHECK_EQ(1, deviceNames.size());
   imgScanner->selectDevice(deviceNames[0]);
#endif

   return imgScanner;
}

TEST(TestImgScanner, acquireFlatbed) {
   unsigned dpi = 100;

   FLAGS_v = 3;
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectSourceAsDefault();

   std::unique_ptr<Image> image = imgScanner->acquireFlatbed(dpi, 0, 0);

   std::pair<float, float> flatbedDimensions;
   imgScanner->getFlatbedDimensionsInInches(flatbedDimensions);

   int width = dpi * flatbedDimensions.first;
   int height = dpi * flatbedDimensions.second;

   // SANE driver does not generate exact dimensions for image
   ASSERT_LE(abs(width - image->getWidth()), IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(height - image->getHeight()), IMAGE_PIXELS_THRESHOLD);
}

TEST(TestImgScanner, acquireImage) {
   unsigned dpi = 600;
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 1.0;

   FLAGS_v = 3;

   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectSourceAsDefault();
   std::unique_ptr<Image> image = imgScanner->acquireImage(dpi, 0, 0, left, top, right, bottom);

   VLOG(2) << "image width: " << image->getWidth() << ", height: " << image->getHeight();

   // SANE driver does not generate exact dimensions for image
   ASSERT_LE(abs(dpi * (right - left) - image->getWidth()), IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(dpi * (bottom - top) - image->getHeight()), IMAGE_PIXELS_THRESHOLD);
}

TEST(TestImgScanner, acquireImageBadBrightness) {
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 1.0;

   FLAGS_v = 3;
   std::pair<int, int> brightnessRange;
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectSourceAsDefault();

   imgScanner->getBrightnessRange(brightnessRange);

   int badBrightness = brightnessRange.first -1;

   EXPECT_DEATH(imgScanner->acquireImage(100, badBrightness, 0, left, top, right, bottom),
                "brightness value is not valid: " + std::to_string(badBrightness));

   badBrightness = brightnessRange.second +1;

   EXPECT_DEATH(imgScanner->acquireImage(100, badBrightness, 0, left, top, right, bottom),
                "brightness value is not valid: " + std::to_string(badBrightness));
}

TEST(TestImgScanner, acquireImageBadContrast) {
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 1.0;

   FLAGS_v = 3;
   std::pair<int, int> contrastRange;
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectSourceAsDefault();

   imgScanner->getContrastRange(contrastRange);

   int badContrast = contrastRange.first -1;

   EXPECT_DEATH(imgScanner->acquireImage(100, 0, badContrast, left, top, right, bottom),
                "contrast value is not valid: " + std::to_string(badContrast));

   badContrast = contrastRange.second +1;

   EXPECT_DEATH(imgScanner->acquireImage(100, 0, badContrast, left, top, right, bottom),
                "contrast value is not valid: " + std::to_string(badContrast));
}

TEST(TestImgScanner, acquireImageBadRegion) {
   FLAGS_v = 3;
   std::pair<float, float> flatbedDimensions;
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectSourceAsDefault();

   imgScanner->getFlatbedDimensionsInInches(flatbedDimensions);

   std::vector<cv::Rect_<float>> regions {
      cv::Rect_<float>(-1.0, 0.0, flatbedDimensions.first, flatbedDimensions.second),
         cv::Rect_<float>(0.0, -1.0, flatbedDimensions.first, flatbedDimensions.second),
         cv::Rect_<float>(0.0, 0.0, flatbedDimensions.first + 0.1, flatbedDimensions.second),
         cv::Rect_<float>(0.0, 0.0, flatbedDimensions.first, flatbedDimensions.second + 0.1),
         cv::Rect_<float>(flatbedDimensions.first, flatbedDimensions.second, 1.0, 1.0)
   };

   for (std::vector<cv::Rect_<float>>::iterator it = regions.begin();
        it != regions.end();
        it++) {

      cv::Rect_<float> & rect = *it;

      VLOG(2) << "rect: " << rect;

      EXPECT_DEATH(imgScanner->acquireImage(100, 0, 0, rect.x, rect.y, rect.width, rect.height),
                   "bounding box exeeds flatbed dimensions");
   }

}

/*

  Add following tests:

   - test for each error condition in imgScanner::acquireImage()

     - bad dpi
     - brightness less than min, greater than max
     - contrast less than min, greater than max
     - bbox dimensions exceed flatbed dimensions

   TODO: this test suite should work for both Linux and Win32

 */

} /* namespace */
