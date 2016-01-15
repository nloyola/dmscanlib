/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/WellDecoder.h"
#include "imgscanner/ImgScanner.h"
#include "test/TestCommon.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

TEST(TestDmScanLib, selectSourceAsDefault) {
    DmScanLib dmScanLib;

#ifdef WIN32
    ASSERT_EQ(SC_SUCCESS, dmScanLib.selectSourceAsDefault());
#else
    ASSERT_EQ(SC_FAIL, dmScanLib.selectSourceAsDefault());
#endif
}

TEST(TestDmScanLib, getValidDpis) {
   std::vector<int> validDpis;
   DmScanLib dmScanLib;

   dmScanLib.selectDevice(test::getFirstDevice().c_str());
   dmScanLib.getValidDpis(validDpis);
   EXPECT_GT(validDpis.size(), 0);

   for (int &dpi : validDpis) {
      VLOG(3) << "dpi: " << dpi;
   }
}

TEST(TestDmScanLib, getFlatbedDimensions) {
   std::pair<float, float> dimensions;
   DmScanLib dmScanLib;

   dmScanLib.selectDevice(test::getFirstDevice().c_str());
   dmScanLib.getFlatbedDimensions(dimensions);

   EXPECT_GT(dimensions.first, 0);
   EXPECT_GT(dimensions.second, 0);

   VLOG(3) << "flatbed dimensions: " << dimensions.first << ", " << dimensions.second;
}

TEST(TestDmScanLib, scanImage) {
   const unsigned dpi = 300;
   float x = 0.400f;
   float y = 0.265f;
   float width = 4.166f;
   float height = 2.755f;

   std::string const fname("tmpscan.png");
   test::deleteFile(fname);

   DmScanLib dmScanLib;
   int result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                    dpi,
                                    0,
                                    0,
                                    x,
                                    y,
                                    x + width,
                                    y + height,
                                    fname.c_str());

   EXPECT_EQ(result, SC_SUCCESS);
   Image image(fname.c_str());

   int expectedWidth = static_cast<int>(static_cast<float>(width * dpi));
   int expectedHeight = static_cast<int>(static_cast<float>(height * dpi));

   ASSERT_LE(abs(expectedWidth - image.getWidth()), test::IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(expectedHeight - image.getHeight()), test::IMAGE_PIXELS_THRESHOLD);
}

TEST(TestDmScanLib, scanBrightnessLimits) {
   int result;
   DmScanLib dmScanLib;
   std::string fname("tmpscan.png");

   int limits[2];
   limits[0] = ImgScanner::MIN_BRIGHTNESS;
   limits[1] = ImgScanner::MAX_BRIGHTNESS;

   for (int & value : limits) {
      result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                   75,
                                   value,
                                   0,
                                   0.0f,
                                   0.0f,
                                   1.0f,
                                   1.0f,
                                   fname.c_str());
      EXPECT_EQ(result, SC_SUCCESS);

      result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                   75,
                                   value,
                                   0,
                                   fname.c_str());
      EXPECT_EQ(result, SC_SUCCESS);
   }
}

TEST(TestDmScanLib, scanInvalidBrightness) {
   int result;
   DmScanLib dmScanLib;
   std::string fname("tmpscan.png");

   int invalidBrightnessValues[2];
   invalidBrightnessValues[0] = ImgScanner::MIN_BRIGHTNESS - 1;
   invalidBrightnessValues[1] = ImgScanner::MAX_BRIGHTNESS + 1;

   for (const int &value : invalidBrightnessValues) {
      result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                   75,
                                   value,
                                   0,
                                   0.0f,
                                   0.0f,
                                   1.0f,
                                   1.0f,
                                   fname.c_str());
      EXPECT_EQ(result, SC_INVALID_BRIGHTNESS);

      result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                   75,
                                   value,
                                   0,
                                   fname.c_str());
      EXPECT_EQ(result, SC_INVALID_BRIGHTNESS);
   }
}

TEST(TestDmScanLib, scanContrastLimits) {
   int result;
   DmScanLib dmScanLib;
   std::string fname("tmpscan.png");

   int limits[2];
   limits[0] = ImgScanner::MIN_CONTRAST;
   limits[1] = ImgScanner::MAX_CONTRAST;

   for (int & value : limits) {
      result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                   75,
                                   0,
                                   value,
                                   0.0f,
                                   0.0f,
                                   1.0f,
                                   1.0f,
                                   fname.c_str());
      EXPECT_EQ(result, SC_SUCCESS);

      result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                   75,
                                   value,
                                   0,
                                   fname.c_str());
      EXPECT_EQ(result, SC_SUCCESS);
   }
}

TEST(TestDmScanLib, scanInvalidContrast) {
   int result;
   DmScanLib dmScanLib;
   std::string fname("tmpscan.png");

   int invalidContrastValues[2];
   invalidContrastValues[0] = ImgScanner::MIN_CONTRAST - 1;
   invalidContrastValues[1] = ImgScanner::MAX_CONTRAST + 1;

   for (const int &value : invalidContrastValues) {
      result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                   75,
                                   0,
                                   value,
                                   0.0f,
                                   0.0f,
                                   1.0f,
                                   1.0f,
                                   fname.c_str());
      EXPECT_EQ(result, SC_INVALID_CONTRAST);

      result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                   75,
                                   0,
                                   value,
                                   fname.c_str());
      EXPECT_EQ(result, SC_INVALID_CONTRAST);
   }
}

TEST(TestDmScanLib, scanFlatbed) {
   std::string fname("flatbed.png");
   test::deleteFile(fname);

   const unsigned dpi = 75;
   DmScanLib dmScanLib;
   int result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                      dpi,
                                      0,
                                      0,
                                      fname.c_str());

   EXPECT_EQ(result, SC_SUCCESS);
   Image image(fname.c_str());

   std::pair<float, float> dimensions;
   dmScanLib.getFlatbedDimensions(dimensions);

   int expectedWidth = static_cast<int>(static_cast<float>(dimensions.first * dpi));
   int expectedHeight = static_cast<int>(static_cast<float>(dimensions.second * dpi));

   ASSERT_LE(abs(expectedWidth - image.getWidth()), test::IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(expectedHeight - image.getHeight()), test::IMAGE_PIXELS_THRESHOLD);
}

TEST(TestDmScanLib, scanNullFilename) {
   std::string fname("flatbed.png");
   test::deleteFile(fname);

   DmScanLib dmScanLib;
   EXPECT_DEATH(dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                    75,
                                    0,
                                    0,
                                    0.0f,
                                    0.0f,
                                    1.0f,
                                    1.0f,
                                    NULL),
		"filename is null");

   EXPECT_DEATH(dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                      75,
                                      0,
                                      0,
                                      NULL),
		"filename is null");
}

TEST(TestDmScanLib, scanBadDpi) {
   std::string fname("flatbed.png");
   test::deleteFile(fname);

   DmScanLib dmScanLib;
   int result;

   result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                0,
                                0,
                                0,
                                0.0f,
                                0.0f,
                                1.0f,
                                1.0f,
                                fname.c_str());
   EXPECT_EQ(result, SC_INVALID_DPI);


   result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                  0,
                                  0,
                                  0,
                                  fname.c_str());
   EXPECT_EQ(result, SC_INVALID_DPI);
}

TEST(TestDmScanLib, scanAndDecode) {
   const unsigned dpi = 600;
   float left = 0.400f;
   float top = 0.265f;
   float width = 4.225f;
   float height = 2.8f;
   std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();

   // make the bounding box with and height one pixel less than the image
   const cv::Rect wellsBbox(0,
                            0,
                            static_cast<unsigned>(width * dpi) - 1,
                            static_cast<unsigned>(height * dpi) - 1);

   std::vector<std::unique_ptr<const WellRectangle> > wellRects;

   test::getWellRectsForBoundingBox(wellsBbox,
                                    8,
                                    12,
                                    LANDSCAPE,
                                    TUBE_BOTTOMS,
                                    wellRects);
   DmScanLib dmScanLib;
   int result = dmScanLib.scanAndDecode(test::getFirstDevice().c_str(),
                                        dpi,
                                        0,
                                        0,
                                        left,
                                        top,
                                        width + left,
                                        height + top,
                                        *decodeOptions,
                                        wellRects);

   EXPECT_EQ(result, SC_SUCCESS);
   EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

   const std::map<std::string, const WellDecoder *> & decodedWells =
      dmScanLib.getDecodedWells();

   for (auto & kv : decodedWells) {
      const dmscanlib::WellDecoder & wellDecoder = *kv.second;
      VLOG(2) << wellDecoder.getLabel() << ": " << wellDecoder.getMessage();
   }

   if (dmScanLib.getDecodedWellCount() > 0) {
      VLOG(2) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
   }
}

TEST(TestDmScanLib, scanAndDecodeMultiple) {
   const unsigned dpi = 300;
   float left = 0.400f;
   float top = 0.265f;
   float width = 4.225f;
   float height = 2.8f;
   std::string firstDevice = test::getFirstDevice();
   std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();

   // make the bounding box with and height one pixel less than the image
   const cv::Rect wellsBbox(0,
                            0,
                            static_cast<unsigned>(width * dpi) - 1,
                            static_cast<unsigned>(height * dpi) - 1);

   std::vector<std::unique_ptr<const WellRectangle> > wellRects;

   test::getWellRectsForBoundingBox(wellsBbox,
                                    8,
                                    12,
                                    LANDSCAPE,
                                    TUBE_BOTTOMS,
                                    wellRects);

   DmScanLib dmScanLib;
   int result = dmScanLib.scanAndDecode(firstDevice.c_str(),
                                        dpi,
                                        0,
                                        0,
                                        left,
                                        top,
                                        width + left,
                                        height + top,
                                        *decodeOptions,
                                        wellRects);

   EXPECT_EQ(result, SC_SUCCESS);
   EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

   std::map<const std::string, std::string> lastResults;
   const std::map<std::string, const WellDecoder *> & decodedWells =
      dmScanLib.getDecodedWells();

   for (auto & kv : decodedWells) {
      const dmscanlib::WellDecoder & wellDecoder = *kv.second;
      lastResults[wellDecoder.getLabel()] = wellDecoder.getMessage();
   }

   result = dmScanLib.scanAndDecode(firstDevice.c_str(),
                                    dpi,
                                    0,
                                    0,
                                    left,
                                    top,
                                    width + left,
                                    height + top,
                                    *decodeOptions,
                                    wellRects);

   const std::map<std::string, const WellDecoder *> & decodedWells2 =
      dmScanLib.getDecodedWells();
   for (auto & kv : decodedWells2) {
      const dmscanlib::WellDecoder & wellDecoder = *kv.second;

      if (lastResults.find(wellDecoder.getLabel()) != lastResults.end()) {
         EXPECT_EQ(lastResults[wellDecoder.getLabel()], wellDecoder.getMessage());
      }
   }
}

TEST(TestDmScanLib, invalidRects) {
   std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
   std::vector<std::unique_ptr<const WellRectangle> > wellRects;
   DmScanLib dmScanLib;
   int result = dmScanLib.decodeImageWells("testImages/8x12/96tubes.bmp", *decodeOptions,
                                           wellRects);
   EXPECT_EQ(result, SC_INVALID_NOTHING_DECODED);
}

TEST(TestDmScanLib, invalidImage) {
   std::unique_ptr<const WellRectangle> wrect(new WellRectangle("label", 0,0,10,10));

   std::vector<std::unique_ptr<const WellRectangle> > wellRects;
   wellRects.push_back(std::move(wrect));

   std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
   DmScanLib dmScanLib;
   int result = dmScanLib.decodeImageWells("xyz.bmp", *decodeOptions, wellRects);
   EXPECT_EQ(result, SC_INVALID_IMAGE);
}

TEST(TestDmScanLib, decodeImage) {
   std::string fname("testImages/8x12/96tubes.bmp");
   //std::string fname("testImages/12x12/stanford_12x12_1.jpg");

   DmScanLib dmScanLib;
   int result = test::decodeImage(fname, dmScanLib, 8, 12);

   EXPECT_EQ(result, SC_SUCCESS);
   EXPECT_GT(dmScanLib.getDecodedWellCount(), 0);

   if (dmScanLib.getDecodedWellCount() > 0) {
      VLOG(2) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
   }
}

} /* namespace */
