/*
 * TestImgScanner.cpp
 *
 *  Created on: Jan 9/2016
 *      Author: nelson
 */

#include "imgscanner/ImgScanner.h"
#include "Image.h"
#include "DmScanLib.h"
#include "test/TestCommon.h"

#include <memory>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

std::unique_ptr<ImgScanner> selectSourceAsDefault() {
   std::unique_ptr<ImgScanner> imgScanner = ImgScanner::create();

#ifndef WIN32
   imgScanner->selectDevice(test::getFirstDevice());
#endif

   return imgScanner;
}

TEST(TestImgScanner, getCapability) {
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();
   EXPECT_NE(imgScanner->getScannerCapability(), 0);
}

TEST(TestImgScanner, acquireFlatbed) {
   unsigned dpi = 75;

   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();

   std::pair<float, float> flatbedDimensions;
   imgScanner->getFlatbedDimensionsInInches(flatbedDimensions);

   std::unique_ptr<Image> image = imgScanner->acquireFlatbed(dpi, 0, 0);

   ASSERT_TRUE(image.get() != NULL)
	   << "no image returned: error: "
	   << imgScanner->getErrorCode();

   VLOG(1) << "image width: " << image->getWidth() << ", height: " << image->getHeight();

   int expectedWidth = static_cast<int>(static_cast<float>(dpi * flatbedDimensions.first));
   int expectedHeight = static_cast<int>(static_cast<float>(dpi * flatbedDimensions.second));

   // SANE driver does not generate exact dimensions for image
   EXPECT_LE(abs(expectedWidth - image->getWidth()), test::IMAGE_PIXELS_THRESHOLD);
   EXPECT_LE(abs(expectedHeight - image->getHeight()), test::IMAGE_PIXELS_THRESHOLD);
}

TEST(TestImgScanner, acquireImage) {
   unsigned dpi = 300;
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 2.0;

   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();
   std::unique_ptr<Image> image =
      imgScanner->acquireImage(dpi, 0, 0, left, top, right, bottom);

   ASSERT_TRUE(image.get() != NULL)
	   << "no image returned: error: "
	   << imgScanner->getErrorCode();

   VLOG(2) << "image width: " << image->getWidth() << ", height: " << image->getHeight();

   int expectedWidth = static_cast<int>(static_cast<float>(dpi * (right - left)));
   int expectedHeight = static_cast<int>(static_cast<float>(dpi * (bottom - top)));

   EXPECT_LE(abs(expectedWidth - image->getWidth()), test::IMAGE_PIXELS_THRESHOLD);
   EXPECT_LE(abs(expectedHeight - image->getHeight()), test::IMAGE_PIXELS_THRESHOLD);
}

TEST(TestImgScanner, acquireImageBadBrightness) {
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 2.0;

   std::pair<int, int> brightnessRange;
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();

   int invalidBrightnessValues[2];
   invalidBrightnessValues[0] = ImgScanner::MIN_BRIGHTNESS - 1;
   invalidBrightnessValues[1] = ImgScanner::MAX_BRIGHTNESS + 1;

   for (const int &value : invalidBrightnessValues) {
      EXPECT_DEATH(
         imgScanner->acquireImage(100, value, 0, left, top, right, bottom),
         "brightness value is not valid: " + std::to_string(value));
   }
}

TEST(TestImgScanner, acquireImageBadContrast) {
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 2.0;

   std::pair<int, int> contrastRange;
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();

   int invalidContrastValues[2];
   invalidContrastValues[0] = ImgScanner::MIN_CONTRAST - 1;
   invalidContrastValues[1] = ImgScanner::MAX_CONTRAST + 1;

   for (const int &value : invalidContrastValues) {
      EXPECT_DEATH(
         imgScanner->acquireImage(100, 0, value, left, top, right, bottom),
         "contrast value is not valid: " + std::to_string(value));
   }
}

TEST(TestImgScanner, acquireImageBadRegion) {
   std::pair<float, float> flatbedDimensions;
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();

   imgScanner->getFlatbedDimensionsInInches(flatbedDimensions);

   std::vector<cv::Rect_<float>> regions;
   regions.push_back(
      cv::Rect_<float>(-1.0l, 0.0l, flatbedDimensions.first, flatbedDimensions.second));
   regions.push_back(
         cv::Rect_<float>(0.0l, -1.0l, flatbedDimensions.first, flatbedDimensions.second));
   regions.push_back(
         cv::Rect_<float>(0.0l, 0.0l, flatbedDimensions.first + 0.1l, flatbedDimensions.second));
   regions.push_back(
         cv::Rect_<float>(0.0l, 0.0l, flatbedDimensions.first, flatbedDimensions.second + 0.1l));
   regions.push_back(
         cv::Rect_<float>(flatbedDimensions.first, flatbedDimensions.second, 1.0l, 1.0l));

   for (cv::Rect_<float> & rect : regions) {
      VLOG(2) << "left: " << rect.x
              << ", top: " << rect.y
              << ", right: " << rect.width
              << ", bottom: " << rect.height;

      EXPECT_DEATH(imgScanner->acquireImage(300, 0, 0, rect.x, rect.y, rect.width, rect.height),
                   "bounding box exeeds flatbed dimensions");
   }

}

} /* namespace */
