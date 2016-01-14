/*
 * TestDmScanLibLinux.cpp
 *
 *  Created on: 2014-01-30
 *      Author: nelson
 */

#include "imgscanner/ImgScannerSane.h"
#include "DmScanLib.h"
#include "test/TestCommon.h"
#include "test/ImageInfo.h"
#include "decoder/WellDecoder.h"
#include "utils/DmTime.h"
#include "Image.h"

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <fstream>

using namespace dmscanlib;

namespace {

class DecodeTestResult {
public:
   bool infoFileValid;
   int decodeResult;
   unsigned totalTubes;
   unsigned totalDecoded;
   double decodeTime;

   DecodeTestResult() {
   }

};

// check that the decoded message matches the one in the "nfo" file
void checkDecodeInfo(DmScanLib & dmScanLib, dmscanlib::test::ImageInfo & imageInfo) {
   const std::map<std::string, const WellDecoder *> & decodedWells = dmScanLib.getDecodedWells();
   for (auto & kv : decodedWells) {
      const WellDecoder & decodedWell = *kv.second;
      const std::string & label = decodedWell.getLabel();
      const std::string * nfoDecodedMsg = imageInfo.getBarcodeMsg(label);

      if ((decodedWell.getMessage().length() > 0) && (nfoDecodedMsg != NULL)) {
         EXPECT_EQ(*nfoDecodedMsg, decodedWell.getMessage()) << "label: " << label;
      }
   }
}

std::unique_ptr<DecodeTestResult> decodeFromInfo(std::string & infoFilename,
                                                 DecodeOptions & decodeOptions,
                                                 DmScanLib & dmScanLib) {

   std::unique_ptr<DecodeTestResult> testResult(new DecodeTestResult());

   dmscanlib::test::ImageInfo imageInfo(infoFilename);
   testResult->infoFileValid = imageInfo.isValid();

   if (testResult->infoFileValid) {
      Image image(imageInfo.getImageFilename());

      VLOG(1) << "image dimensions: width: " << image.getWidth()
              << ", height: " << image.getHeight();

      testResult->totalTubes = imageInfo.getDecodedWellCount();
      std::vector<std::unique_ptr<const WellRectangle> > wellRects;
      test::getWellRectsForBoundingBox(imageInfo.getBoundingBox(),
                                       imageInfo.getPalletRows(),
                                       imageInfo.getPalletCols(),
                                       imageInfo.getOrientation(),
                                       imageInfo.getBarcodePosition(),
                                       wellRects);

      if (!wellRects.empty()) {
         util::DmTime start;
         testResult->decodeResult =
            dmScanLib.decodeImageWells(imageInfo.getImageFilename().c_str(),
                                       decodeOptions,
                                       wellRects);
         util::DmTime end;

         testResult->decodeTime = end.difftime(start)->getTime();

         if (testResult->decodeResult == SC_SUCCESS) {
         testResult->totalDecoded = dmScanLib.getDecodedWellCount();
         checkDecodeInfo(dmScanLib, imageInfo);
         }
      } else {
         LOG(WARNING) << "well rectangles could not be determined";
         testResult->decodeResult = SC_INVALID_NOTHING_DECODED;
      }
   }

   return testResult;
}

void writeAllDecodeResults(std::vector<std::string> & testResults, bool append = false) {
   std::ofstream ofile;
   if (append) {
      ofile.open("all_images_results.csv", std::ios::app);
   } else {
      ofile.open("all_images_results.csv");
   }

   for (unsigned i = 0, n = testResults.size(); i < n; ++i) {
      ofile <<  testResults[i] << std::endl;
   }
   ofile.close();
}

TEST(TestDmScanLibLinux, readInfoFiles) {
   FLAGS_v = 1;

   std::string dirname("testImageInfo");
   std::vector<std::string> filenames;
   bool result = test::getTestImageInfoFilenames(dirname, filenames);
   EXPECT_EQ(true, result);

   std::stringstream ss;
   std::vector<std::string> testResults;
   testResults.push_back("#filename,decoded,total,ratio,time (sec)");

   for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
      ss.str("");
      VLOG(1) << "test image info: " << filenames[i];

      dmscanlib::test::ImageInfo imageInfo(filenames[i]);
      bool result = imageInfo.isValid();
      EXPECT_EQ(true, result);
   }

}

TEST(TestDmScanLibLinux, decodeFromInfo) {
   FLAGS_v = 1;

   //std::string infoFilename("testImageInfo/8x12/calgary2.nfo");
   //std::string infoFilename("testImageInfo/8x12/new_tubes.nfo");
   //std::string infoFilename("testImageInfo/9x9/stanford_9x9_1.nfo");
   //std::string infoFilename("testImageInfo/8x12/FrozenPalletImages/HP_L1985A/scanned1_1.nfo");
   //std::string infoFilename("testImageInfo/8x12/problem_tubes_portrait.nfo");
   std::string infoFilename("testImageInfo/8x12/edge_tubes.nfo");

   std::unique_ptr<DecodeOptions> defaultDecodeOptions = test::getDefaultDecodeOptions();
   DecodeOptions decodeOptions(0.2,
                               0.4,
                               0.0,
                               defaultDecodeOptions->squareDev,
                               defaultDecodeOptions->edgeThresh,
                               defaultDecodeOptions->corrections,
                               defaultDecodeOptions->shrink);

   DmScanLib dmScanLib(0);
   std::unique_ptr<DecodeTestResult> testResult = decodeFromInfo(infoFilename,
                                                                 decodeOptions,
                                                                 dmScanLib);

   EXPECT_TRUE(testResult->infoFileValid);
   EXPECT_EQ(SC_SUCCESS, testResult->decodeResult);
   if (testResult->decodeResult == SC_SUCCESS) {
      EXPECT_TRUE(testResult->totalDecoded > 0);

      VLOG(1) << "total: " << testResult->totalTubes
              << ", decoded: " << testResult->totalDecoded
              << ", time taken: " << testResult->decodeTime;

   }
}

//TEST(TestDmScanLib, DISABLED_decodeAllImages) {
TEST(TestDmScanLibLinux, decodeAllImages) {
   FLAGS_v = 1;

   std::string dirname("testImageInfo");
   std::vector<std::string> filenames;
   bool result = test::getTestImageInfoFilenames(dirname, filenames);
   EXPECT_EQ(true, result);

   std::vector<std::string> testResults;
   testResults.push_back("#filename,decoded,total,ratio,time (sec)");

   unsigned totalTubes = 0;
   unsigned totalDecoded = 0;
   double totalTime = 0;
   std::stringstream ss;
   std::unique_ptr<DecodeOptions> defaultDecodeOptions = test::getDefaultDecodeOptions();
   DecodeOptions decodeOptions(0.15,
                               0.3,
                               0.1,
                               defaultDecodeOptions->squareDev,
                               defaultDecodeOptions->edgeThresh,
                               defaultDecodeOptions->corrections,
                               defaultDecodeOptions->shrink);

   for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
      ss.str("");
      VLOG(1) << "test image info: " << filenames[i];

      DmScanLib dmScanLib(0);
      std::unique_ptr<DecodeTestResult> testResult =
         decodeFromInfo(filenames[i], decodeOptions, dmScanLib);
      EXPECT_TRUE(testResult->infoFileValid);
      EXPECT_EQ(SC_SUCCESS, testResult->decodeResult);

      ss << filenames[i];

      if (testResult->decodeResult == SC_SUCCESS) {
         EXPECT_TRUE(testResult->totalDecoded > 0);

         ss << "," << testResult->totalDecoded << ","
            << testResult->totalTubes << ","
            << testResult->totalDecoded / static_cast<float>(testResult->totalTubes) << ","
            << testResult->decodeTime;

         VLOG(1) << "decoded: " << testResult->totalDecoded
                 << ", total: " << testResult->totalTubes
                 << ", time taken: " << testResult->decodeTime;

         totalDecoded += testResult->totalDecoded;
         totalTubes += testResult->totalTubes;
         totalTime += testResult->decodeTime;
      }
      testResults.push_back(ss.str());
   }

   ss.str("");
   ss << "decoded:," << totalDecoded << ", total:," << totalTubes;
   testResults.push_back(ss.str());
   ss.str("");

   double avgDecodeTime = totalTime / filenames.size();
   ss << "average decode time:," << avgDecodeTime;
   testResults.push_back(ss.str());

   VLOG(1) << "total tubes: " << totalTubes
           << ", total decoded: " << totalDecoded
           << ", average decode time: " << avgDecodeTime;

   writeAllDecodeResults(testResults);
}

TEST(TestDmScanLibLinux, DISABLED_decodeAllImagesAllParameters) {
//TEST(TestDmScanLib, decodeAllImagesAllParameters) {
   FLAGS_v = 1;

   std::string dirname("testImageInfo");
   std::vector<std::string> filenames;
   bool result = test::getTestImageInfoFilenames(dirname, filenames);
   EXPECT_EQ(true, result);

   std::unique_ptr<DecodeOptions> defaultDecodeOptions = test::getDefaultDecodeOptions();
   std::vector<std::string> testResults;
   testResults.push_back("#filename,decoded,total,ratio,time (sec),minEdgeFactor,maxEdgeFactor,scanGapFactor");
   writeAllDecodeResults(testResults, false); // clear previous contents of the file

   std::stringstream ss;

   for (double maxEdge = 0.15; maxEdge <= 1.05; maxEdge += 0.05) {
      for (double minEdge = 0, n = maxEdge + 0.05; minEdge < n; minEdge += 0.05) {
         for (double scanGap = 0; scanGap <= 1.05; scanGap += 0.05) {
            unsigned totalTubes = 0;
            unsigned totalDecoded = 0;
            double totalTime = 0;

            testResults.clear();

            DecodeOptions decodeOptions(minEdge,
                                        maxEdge,
                                        scanGap,
                                        defaultDecodeOptions->squareDev,
                                        defaultDecodeOptions->edgeThresh,
                                        defaultDecodeOptions->corrections,
                                        defaultDecodeOptions->shrink);

            for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
               ss.str("");
               VLOG(1) << "test image info: " << filenames[i];

               DmScanLib dmScanLib(0);
               std::unique_ptr<DecodeTestResult> testResult =
                  decodeFromInfo(filenames[i], decodeOptions, dmScanLib);
               EXPECT_TRUE(testResult->infoFileValid);
               EXPECT_EQ(SC_SUCCESS, testResult->decodeResult);

               ss << filenames[i];

               if (testResult->decodeResult == SC_SUCCESS) {
                  EXPECT_TRUE(testResult->totalDecoded > 0);

                  ss << "," << testResult->totalDecoded
                     << "," << testResult->totalTubes
                     << "," << testResult->totalDecoded / static_cast<float>(testResult->totalTubes)
                     << "," << testResult->decodeTime
                     << "," << minEdge
                     << "," << maxEdge
                     << "," << scanGap;

                  VLOG(1) << "decoded: " << testResult->totalDecoded
                          << ", total: " << testResult->totalTubes
                          << ", time taken: " << testResult->decodeTime
                          << ", minEdgeFactor: " << minEdge
                          << ", maxedgeFactor: " << maxEdge
                          << ", scanGapFactor: " << scanGap;

                  totalDecoded += testResult->totalDecoded;
                  totalTubes += testResult->totalTubes;
                  totalTime += testResult->decodeTime;
               }

               testResults.push_back(ss.str());
            }

            double avgDecodeTime = totalTime / filenames.size();
            ss.str("");
            ss << "RESULTS:,decoded:," << totalDecoded
               << ", total:," << totalTubes
               << ", average decode time:," << avgDecodeTime
               << ", minEdgeFactor:," << minEdge
               << ", maxedgeFactor:," << maxEdge
               << ", scanGapFactor:," << scanGap;
            testResults.push_back(ss.str());
            writeAllDecodeResults(testResults, true);
         }
      }
   }
}

} /* namespace */
