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

int IMAGE_PIXELS_THRESHOLD = 5;

std::unique_ptr<ImgScanner> selectSourceAsDefault() {
   std::unique_ptr<ImgScanner> imgScanner = ImgScanner::create();

#ifndef WIN32
   imgScanner->selectDevice(test::getFirstDevice());
#endif

   return imgScanner;
}

TEST(TestImgScanner, getCapability) {
   FLAGS_v = 5;
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();
   EXPECT_NE(imgScanner->getScannerCapability(), 0);
}

TEST(TestImgScanner, acquireFlatbed) {
   unsigned dpi = 75;

   FLAGS_v = 0;
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();

   std::pair<float, float> flatbedDimensions;
   imgScanner->getFlatbedDimensionsInInches(flatbedDimensions);

   std::unique_ptr<Image> image = imgScanner->acquireFlatbed(dpi, 0, 0);

   ASSERT_TRUE(image.get() != NULL)
	   << "no image returned: error: "
	   << imgScanner->getErrorCode();

   VLOG(2) << "image width: " << image->getWidth() << ", height: " << image->getHeight();

   int expectedWidth = static_cast<int>(static_cast<float>(dpi * flatbedDimensions.first));
   int expectedHeight = static_cast<int>(static_cast<float>(dpi * flatbedDimensions.second));

   // SANE driver does not generate exact dimensions for image
   EXPECT_LE(abs(expectedWidth - image->getWidth()), IMAGE_PIXELS_THRESHOLD);
   EXPECT_LE(abs(expectedHeight - image->getHeight()), IMAGE_PIXELS_THRESHOLD);
}

TEST(TestImgScanner, acquireImage) {
   unsigned dpi = 300;
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 2.0;

   FLAGS_v = 0;

   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();
   std::unique_ptr<Image> image =
      imgScanner->acquireImage(dpi, 0, 0, left, top, right, bottom);

   ASSERT_TRUE(image.get() != NULL)
	   << "no image returned: error: "
	   << imgScanner->getErrorCode();

   VLOG(2) << "image width: " << image->getWidth() << ", height: " << image->getHeight();

   int expectedWidth = static_cast<int>(static_cast<float>(dpi * (right - left)));
   int expectedHeight = static_cast<int>(static_cast<float>(dpi * (bottom - top)));

   EXPECT_LE(abs(expectedWidth - image->getWidth()), IMAGE_PIXELS_THRESHOLD);
   EXPECT_LE(abs(expectedHeight - image->getHeight()), IMAGE_PIXELS_THRESHOLD);
}

TEST(TestImgScanner, acquireImageBadBrightness) {
   float left = 1.0;
   float top = 1.0;
   float right = 3.0;
   float bottom = 1.0;

   FLAGS_v = 3;
   std::pair<int, int> brightnessRange;
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();

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

   FLAGS_v = 0;
   std::pair<int, int> contrastRange;
   std::unique_ptr<ImgScanner> imgScanner = selectSourceAsDefault();

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

   for (std::vector<cv::Rect_<float>>::iterator it = regions.begin();
        it != regions.end();
        it++) {

      cv::Rect_<float> & rect = *it;

	  VLOG(2) << "left: " << rect.x
		  << ", top: " << rect.y
		  << ", right: " << rect.width
		  << ", bottom: " << rect.height;

      EXPECT_DEATH(imgScanner->acquireImage(300, 0, 0, rect.x, rect.y, rect.width, rect.height),
                   "bounding box exeeds flatbed dimensions");
   }

}

} /* namespace */
