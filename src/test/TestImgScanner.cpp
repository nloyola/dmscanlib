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

   std::pair<double, double> flatbedDimensions;
   imgScanner->getFlatbedDimensionsInInches(flatbedDimensions);

   int width = dpi * flatbedDimensions.first;
   int height = dpi * flatbedDimensions.second;

   // SANE driver does not generate exact dimensions for image
   ASSERT_LE(abs(width - image->getWidth()), IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(height - image->getHeight()), IMAGE_PIXELS_THRESHOLD);
}

TEST(TestImgScanner, acquireImage) {
   unsigned dpi = 600;
   float x = 1.0;
   float y = 1.0;
   float width = 3.0;
   float height = 1.0;
   cv::Rect_<float> scanRect(x, y, x + width, y + height);

   FLAGS_v = 3;

   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectSourceAsDefault();
   std::unique_ptr<Image> image = imgScanner->acquireImage(dpi, 0, 0, scanRect);

   VLOG(2) << "image width: " << image->getWidth() << ", height: " << image->getHeight();

   // SANE driver does not generate exact dimensions for image
   ASSERT_LE(abs(dpi * width - image->getWidth()), IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(dpi * height - image->getHeight()), IMAGE_PIXELS_THRESHOLD);
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
