/*
 * TestImgScannerLib.cpp
 *
 *  Created on: 2014-01-30
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

std::unique_ptr<imgscanner::ImgScannerSane> selectFirstDevice() {
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner(new imgscanner::ImgScannerSane());
   std::vector<std::string> deviceNames;
   imgScanner->getDeviceNames(deviceNames);
   CHECK_EQ(1, deviceNames.size());
   imgScanner->selectDevice(deviceNames[0]);
   return imgScanner;
}

TEST(TestImgScannerLinux, selectDevice) {
   FLAGS_v = 3;
   std::vector<std::string> deviceNames;

   imgscanner::ImgScannerSane imgScanner;
   imgScanner.getDeviceNames(deviceNames);

   ASSERT_EQ(1, deviceNames.size());
   // change device name to yours
   ASSERT_NE(std::string::npos, deviceNames[0].find("psc_2400"));

   imgScanner.selectDevice(deviceNames[0]);
}

TEST(TestImgScannerLinux, getCapabilityBadDeviceName) {
   FLAGS_v = 3;
   imgscanner::ImgScannerSane imgScanner;
   imgScanner.selectDevice("badDeviceName");
   EXPECT_DEATH(imgScanner.getScannerCapability(),
                "SANE open failed: " + std::to_string(SANE_STATUS_INVAL));
}

TEST(TestImgScannerLinux, getCapabilityNoDeviceSelected) {
   FLAGS_v = 3;
   imgscanner::ImgScannerSane imgScanner;
   EXPECT_DEATH(imgScanner.getScannerCapability(), "no device selected");
}

TEST(TestImgScannerLinux, getCapability) {
   FLAGS_v = 3;
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectFirstDevice();
   ASSERT_NE(0, imgScanner->getScannerCapability());
}

TEST(TestImgScannerLinux, acquireImageNoDeviceSelected) {
   FLAGS_v = 3;
   cv::Rect_<float> scanRect(0, 0, 1, 1);
   imgscanner::ImgScannerSane imgScanner;
   EXPECT_DEATH(imgScanner.acquireImage(100, 0, 0, scanRect), "no device selected");
}

TEST(TestImgScannerLinux, acquireFlatbedNoDeviceSelected) {
   FLAGS_v = 3;
   imgscanner::ImgScannerSane imgScanner;
   EXPECT_DEATH(imgScanner.acquireFlatbed(100, 0, 0), "no device selected");
}

TEST(TestImgScannerLinux, getFlatbedDimensionsInInchesNoDeviceSelected) {
   FLAGS_v = 3;
   imgscanner::ImgScannerSane imgScanner;
   std::pair<double, double> pair;
   EXPECT_DEATH(imgScanner.getFlatbedDimensionsInInches(pair), "no device selected");
}

TEST(TestImgScannerLinux, getFlatbedDimensions) {
   std::unique_ptr<imgscanner::ImgScannerSane> imgScanner = selectFirstDevice();
   std::pair<double, double> pair;
   imgScanner->getFlatbedDimensionsInInches(pair);
   ASSERT_GT(pair.first, 0.0);
   ASSERT_GT(pair.second, 0.0);
}

TEST(TestImgScannerLinux, getBrightnessRangeNoDeviceSelected) {
   FLAGS_v = 3;
   imgscanner::ImgScannerSane imgScanner;
   std::pair<double, double> range;
   EXPECT_DEATH(imgScanner.getBrightnessRange(range), "no device selected");
}

TEST(TestImgScannerLinux, acquireImageBadBrightness) {
   FLAGS_v = 3;
   cv::Rect_<float> scanRect(0, 0, 1, 1);
   imgscanner::ImgScannerSane imgScanner;
   EXPECT_DEATH(imgScanner.acquireImage(100, 0, 0, scanRect), "no device selected");
}

} /* namespace */
