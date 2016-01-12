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
#include "test/TestCommon.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

int IMAGE_PIXELS_THRESHOLD = 5;

/**
 * Do not care if call to delete file fails.
 */
void deleteFile(const std::string & filename) {
#ifdef WIN32
   std::wstring fnamew(filename.begin(), filename.end());
   DeleteFile(fnamew.c_str());
#else
   remove(filename.c_str());
#endif
}

TEST(TestDmScanLib, selectSourceAsDefault) {
    FLAGS_v = 1;
    DmScanLib dmScanLib(1);

#ifdef WIN32
    ASSERT_EQ(SC_SUCCESS, dmScanLib.selectSourceAsDefault());
#else
    ASSERT_EQ(SC_FAIL, dmScanLib.selectSourceAsDefault());
#endif
}

TEST(TestDmScanLib, scanImage) {
   FLAGS_v = 1;

   const unsigned dpi = 300;
   float x = 0.400f;
   float y = 0.265f;
   float width = 4.166f;
   float height = 2.755f;

   std::string const fname("tmpscan.png");
   deleteFile(fname);

   DmScanLib dmScanLib(1);
   int result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                    dpi,
                                    0,
                                    0,
                                    x,
                                    y,
                                    x + width,
                                    y + height,
                                    fname.c_str());

   EXPECT_EQ(SC_SUCCESS, result);
   Image image(fname.c_str());

   int expectedWidth = static_cast<int>(static_cast<float>(width * dpi));
   int expectedHeight = static_cast<int>(static_cast<float>(height * dpi));

   ASSERT_LE(abs(expectedWidth - image.getWidth()), IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(expectedHeight - image.getHeight()), IMAGE_PIXELS_THRESHOLD);
}

TEST(TestDmScanLib, scanImageBadParams) {
   FLAGS_v = 1;

   DmScanLib dmScanLib(1);
   EXPECT_DEATH(dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                    300,
                                    0,
                                    0,
                                    0.0f,
                                    0.0f,
                                    1.0f,
                                    1.0f,
                                    NULL),
		"filename is null");
}

TEST(TestDmScanLib, scanImageInvalidDpi) {
   FLAGS_v = 1;

   DmScanLib dmScanLib(1);
   std::string fname("tmpscan.png");

   int result = dmScanLib.scanImage(test::getFirstDevice().c_str(),
                                    0,
                                    0,
                                    0,
                                    0.0f,
                                    0.0f,
                                    1.0f,
                                    1.0f,
                                    fname.c_str());
   EXPECT_EQ(SC_INVALID_DPI, result);
}

TEST(TestDmScanLib, scanFlatbed) {
   FLAGS_v = 1;

   std::string fname("flatbed.png");
   deleteFile(fname);

   const unsigned dpi = 75;
   DmScanLib dmScanLib(1);
   int result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                      dpi,
                                      0,
                                      0,
                                      fname.c_str());

   EXPECT_EQ(SC_SUCCESS, result);
   Image image(fname.c_str());

   std::pair<float, float> dimensions;
   dmScanLib.getFlatbedDimensions(dimensions);

   int expectedWidth = static_cast<int>(static_cast<float>(dimensions.first * dpi));
   int expectedHeight = static_cast<int>(static_cast<float>(dimensions.second * dpi));

   ASSERT_LE(abs(expectedWidth - image.getWidth()), IMAGE_PIXELS_THRESHOLD);
   ASSERT_LE(abs(expectedHeight - image.getHeight()), IMAGE_PIXELS_THRESHOLD);
}

TEST(TestDmScanLib, scanFlatbedBadParams) {
   FLAGS_v = 1;

   std::string fname("flatbed.png");
   deleteFile(fname);

   DmScanLib dmScanLib(1);
   EXPECT_DEATH(dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                      75,
                                      0,
                                      0,
                                      NULL),
		"filename is null");

   int result = dmScanLib.scanFlatbed(test::getFirstDevice().c_str(),
                                      0,
                                      0,
                                      0,
                                      fname.c_str());
   EXPECT_EQ(SC_INVALID_DPI, result);
}

TEST(TestDmScanLib, scanAndDecode) {
   FLAGS_v = 1;

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
   DmScanLib dmScanLib(1);
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

   EXPECT_EQ(SC_SUCCESS, result);
   EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

   const std::map<std::string, const WellDecoder *> & decodedWells =
      dmScanLib.getDecodedWells();

   for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
        ii != decodedWells.end(); ++ii) {
      const dmscanlib::WellDecoder & wellDecoder = *(ii->second);
      VLOG(1) << wellDecoder.getLabel() << ": " << wellDecoder.getMessage();
   }

   if (dmScanLib.getDecodedWellCount() > 0) {
      VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
   }
}

TEST(TestDmScanLib, scanAndDecodeMultiple) {
   FLAGS_v = 1;

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

   DmScanLib dmScanLib(1);
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

   EXPECT_EQ(SC_SUCCESS, result);
   EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

   std::map<const std::string, std::string> lastResults;
   const std::map<std::string, const WellDecoder *> & decodedWells =
      dmScanLib.getDecodedWells();

   for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
        ii != decodedWells.end(); ++ii) {
      const dmscanlib::WellDecoder & wellDecoder = *(ii->second);
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
   for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells2.begin();
        ii != decodedWells2.end(); ++ii) {
      const dmscanlib::WellDecoder & wellDecoder = *(ii->second);

      if (lastResults.find(wellDecoder.getLabel()) != lastResults.end()) {
         EXPECT_EQ(lastResults[wellDecoder.getLabel()], wellDecoder.getMessage());
      }
   }
}

TEST(TestDmScanLib, invalidRects) {
   FLAGS_v = 0;

   std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
   std::vector<std::unique_ptr<const WellRectangle> > wellRects;
   DmScanLib dmScanLib(1);
   int result = dmScanLib.decodeImageWells("testImages/8x12/96tubes.bmp", *decodeOptions,
                                           wellRects);
   EXPECT_EQ(SC_INVALID_NOTHING_DECODED, result);
}

TEST(TestDmScanLib, invalidImage) {
   FLAGS_v = 0;

   std::unique_ptr<const WellRectangle> wrect(new WellRectangle("label", 0,0,10,10));

   std::vector<std::unique_ptr<const WellRectangle> > wellRects;
   wellRects.push_back(std::move(wrect));

   std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
   DmScanLib dmScanLib(1);
   int result = dmScanLib.decodeImageWells("xyz.bmp", *decodeOptions, wellRects);
   EXPECT_EQ(SC_INVALID_IMAGE, result);
}

TEST(TestDmScanLib, decodeImage) {
   FLAGS_v = 1;

   std::string fname("testImages/8x12/96tubes.bmp");
   //std::string fname("testImages/12x12/stanford_12x12_1.jpg");

   DmScanLib dmScanLib(1);
   int result = test::decodeImage(fname, dmScanLib, 8, 12);

   EXPECT_EQ(result, SC_SUCCESS);
   EXPECT_GT(dmScanLib.getDecodedWellCount(), 0);

   if (dmScanLib.getDecodedWellCount() > 0) {
      VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
   }
}

} /* namespace */
