/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "DmScanLib.h"
#include "imgscanner/ImgScanner.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"
#include "Image.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

namespace dmscanlib {

const std::string DmScanLib::LIBRARY_NAME("dmscanlib");

bool DmScanLib::loggingInitialized = false;

DmScanLib::DmScanLib() :
      imgScanner(std::move(ImgScanner::create()))
{
}

DmScanLib::DmScanLib(unsigned loggingLevel, bool logToFile) :
      imgScanner(std::move(ImgScanner::create()))
{
   configLogging(loggingLevel, logToFile);
}

DmScanLib::~DmScanLib() {
}

int DmScanLib::selectSourceAsDefault() {
   if (imgScanner->selectSourceAsDefault()) {
      return SC_SUCCESS;
   }
   return SC_FAIL;
}

void DmScanLib::getDeviceNames(std::vector<std::string> & names) {
   imgScanner->getDeviceNames(names);
}

void DmScanLib::selectDevice(const char * const deviceName) {
   imgScanner->selectDevice(deviceName);
}

void DmScanLib::getValidDpis(std::vector<int> & validDpis) {
   imgScanner->getValidDpis(validDpis);
}

int DmScanLib::getScannerCapability() {
   return imgScanner->getScannerCapability();
}

void DmScanLib::getFlatbedDimensions(std::pair<float, float> & dimensions) {
   imgScanner->getFlatbedDimensionsInInches(dimensions);
}

void DmScanLib::configLogging(unsigned level, bool useFile) {
   if (loggingInitialized)
      return;

   google::InitGoogleLogging(LIBRARY_NAME.c_str());
   FLAGS_v = level;
   FLAGS_stderrthreshold = (level > 0) ? google::GLOG_INFO : google::GLOG_ERROR;

   FLAGS_logtostderr = !useFile;
   FLAGS_alsologtostderr = false;

   loggingInitialized = true;
}

int DmScanLib::scanImage(const char * const deviceName,
                         unsigned dpi,
                         int brightness,
                         int contrast,
                         float left,
                         float top,
                         float right,
                         float bottom,
                         const char * const filename) {

   VLOG(1) << "scanImage: deviceName: " << deviceName
           << ", dpi:" << dpi
           << ", brightness:" << brightness
           << ", contrast:" << contrast
           << ", left: " << left
           << ", top: " << top
           << ", right: " << right
           << ", bottom: " << bottom
           << ", filename:" << filename;

   CHECK(filename != NULL) << "filename is null";

   imgScanner->selectDevice(deviceName);
   std::unique_ptr<Image> image = imgScanner->acquireImage(dpi,
                                                          brightness,
                                                          contrast,
                                                          left,
                                                          top,
                                                          right,
                                                          bottom);
   if (image.get() == NULL) {
      VLOG(1) << "could not acquire image";
      return imgScanner->getErrorCode();
   }

   image->write(filename);
   return SC_SUCCESS;
}

int DmScanLib::scanFlatbed(const char * const deviceName,
                           unsigned dpi,
                           int brightness,
                           int contrast,
                           const char * const filename) {
   VLOG(1) << "scanFlatbed: deviceName: " << deviceName
           << ", dpi:" << dpi
           << ", brightness:" << brightness
           << ", contrast:" << contrast
           << ", filename:" << filename;

   CHECK(filename != NULL) << "filename is null";

   imgScanner->selectDevice(deviceName);
   std::unique_ptr<Image> image = imgScanner->acquireFlatbed(dpi, brightness, contrast);
   if (image.get() == NULL) {
      VLOG(1) << "could not acquire image";
      return imgScanner->getErrorCode();
   }
   image->write(filename);
   return SC_SUCCESS;
}

int DmScanLib::scanAndDecode(const char * const deviceName,
                             unsigned dpi,
                             int brightness,
                             int contrast,
                             float left,
                             float top,
                             float right,
                             float bottom,
                             const DecodeOptions & decodeOptions,
                             std::vector<std::unique_ptr<const WellRectangle> > & wellRects) {
   VLOG(3) << "scanAndDecode:  deviceName: " << deviceName
           << ", dpi:" << dpi
           << ", brightness:" << brightness
           << ", contrast:" << contrast
           << ", left: " << left
           << ", top: " << top
           << ", right: " << right
           << ", bottom: " << bottom
           << ", " << decodeOptions;

   int result;

   imgScanner->selectDevice(deviceName);
   std::unique_ptr<Image> image = imgScanner->acquireImage(dpi,
                                                          brightness,
                                                          contrast,
                                                          left,
                                                          top,
                                                          right,
                                                          bottom);
   if (image.get() == NULL) {
      VLOG(1) << "could not acquire image";
      return imgScanner->getErrorCode();
   }
   image->write("scanned.png");
   result = decodeCommon(*image, decodeOptions, "decode.png", wellRects);

   VLOG(1) << "decodeCommon returned: " << result;
   return result;
}

int DmScanLib::decodeImageWells(const char * const filename,
                                const DecodeOptions & decodeOptions,
                                std::vector<std::unique_ptr<const WellRectangle> > & wellRects) {

   VLOG(1) << "decodeImageWells: filename:" << filename
           << ", numWellRects:" << wellRects.size()
           << ", " << decodeOptions;

   Image image(filename);
   if (!image.isValid()) {
      return SC_INVALID_IMAGE;
   }

   return decodeCommon(image, decodeOptions, "decode.png", wellRects);
}

int DmScanLib::decodeCommon(const Image & image,
                            const DecodeOptions & decodeOptions,
                            std::string const & decodedDibFilename,
                            std::vector<std::unique_ptr<const WellRectangle> > & wellRects) {

   decoder = std::unique_ptr<Decoder>(new Decoder(image, decodeOptions, wellRects));
   int result = decoder->decodeWellRects();

   if (result != SC_SUCCESS) {
      return result;
   }

   const unsigned decodedWellCount = decoder->getDecodedWellCount();

   if (decodedWellCount == 0) {
      return SC_INVALID_NOTHING_DECODED;
   }

   writeDecodedImage(image, decodedDibFilename);

   return SC_SUCCESS;
}

void DmScanLib::writeDecodedImage(const Image & image,
                                  const std::string & decodedDibFilename) {

   CHECK_NOTNULL(decoder.get());

   std::vector<std::unique_ptr<WellDecoder> > & wellDecoders = decoder->getWellDecoders();
   CHECK(wellDecoders.size() > 0);

   const std::map<std::string, const WellDecoder *> & decodedWells = decoder->getDecodedWells();
   CHECK(decodedWells.size() > 0);

   cv::Scalar colorBlue(0, 0, 255);
   cv::Scalar colorRed(255, 0, 0);
   cv::Scalar colorGreen(0, 255, 0);

   Image decodedImage(image);

   for (unsigned i = 0, n = wellDecoders.size(); i < n; ++i) {
      decodedImage.drawRectangle(wellDecoders[i]->getWellRectangle(), colorBlue);
   }

   for (auto & kv : decodedWells) {
      const WellDecoder & decodedWell = *kv.second;
      const std::vector<cv::Point> & bboxDecoded = decodedWell.getDecodedQuad();

      decodedImage.drawLine(bboxDecoded[0], bboxDecoded[1], colorRed);
      decodedImage.drawLine(bboxDecoded[1], bboxDecoded[2], colorRed);
      decodedImage.drawLine(bboxDecoded[2], bboxDecoded[3], colorRed);
      decodedImage.drawLine(bboxDecoded[3], bboxDecoded[0], colorRed);

      const cv::Rect & wellBbox = decodedWell.getWellRectangle();
      decodedImage.drawRectangle(wellBbox, colorGreen);
   }
   decodedImage.write(decodedDibFilename.c_str());
}

const unsigned DmScanLib::getDecodedWellCount() {
   if (decoder == NULL) {
      throw std::logic_error("decoder is null");
   }
   return decoder->getDecodedWellCount();
}

const std::map<std::string, const WellDecoder *> & DmScanLib::getDecodedWells() const {
   CHECK_NOTNULL(decoder.get());
   return decoder->getDecodedWells();
}

Orientation DmScanLib::getOrientationFromString(std::string & orientationStr) {
   Orientation orientation = ORIENTATION_MAX;

   if (orientationStr.compare("landscape") == 0) {
      orientation = LANDSCAPE;
   } else if (orientationStr.compare("portrait") == 0) {
      orientation = PORTRAIT;
   }

   return orientation;
}

BarcodePosition DmScanLib::getBarcodePositionFromString(std::string & positionStr) {
   BarcodePosition position = BARCODE_POSITION_MAX;

   if (positionStr.compare("top") == 0) {
      position = TUBE_TOPS;
   } else if (positionStr.compare("bottom") == 0) {
      position = TUBE_BOTTOMS;
   }

   return position;
}

std::string DmScanLib::sbsLabelingFromRowCol(unsigned row, unsigned col) {
   std::ostringstream label;
   label << (char) ('A' + row) << (col  + 1);
   return label.str();
}

std::string DmScanLib::getLabelForPosition(unsigned row,
                                           unsigned col,
                                           unsigned rowsMax,
                                           unsigned colsMax,
                                           Orientation orientation,
                                           BarcodePosition barcodePosition) {

   switch (barcodePosition) {
      case TUBE_TOPS:
         switch (orientation) {
            case LANDSCAPE:
               return sbsLabelingFromRowCol(row, col);
            case PORTRAIT:
               return sbsLabelingFromRowCol(colsMax - 1 - col, row);

            default:
               CHECK(false) << "invalid value for orientation: " << orientation;
         }
         break;

      case TUBE_BOTTOMS:
         switch (orientation) {
            case LANDSCAPE:
               return sbsLabelingFromRowCol(row, colsMax - 1 - col);
            case PORTRAIT:
               return sbsLabelingFromRowCol(col, row);

            default:
               CHECK(false) << "invalid value for orientation: " << orientation;
         }
         break;

      default:
         CHECK(false) << "invalid value for barcode position: " << barcodePosition;
   }
   return NULL;
}


PalletSize DmScanLib::getPalletSizeFromString(std::string & palletSizeStr) {
   PalletSize palletSize = PSIZE_MAX;

   if (palletSizeStr.compare("8x12") == 0) {
      palletSize = PSIZE_8x12;
   } else if (palletSizeStr.compare("10x10") == 0) {
      palletSize = PSIZE_10x10;
   } else if (palletSizeStr.compare("12x12") == 0) {
      palletSize = PSIZE_12x12;
   } else if (palletSizeStr.compare("9x9") == 0) {
      palletSize = PSIZE_9x9;
   } else if (palletSizeStr.compare("1x1") == 0) {
      palletSize = PSIZE_1x1;
   }

   return palletSize;
}

std::ostream & operator<<(std::ostream &os, Orientation m) {
   switch (m) {
      case LANDSCAPE: os << "landscape"; break;
      case PORTRAIT: os << "portrait"; break;
      default:
         throw std::logic_error("invalid value for orientation");
   }
   return os;
}

std::ostream & operator<<(std::ostream &os, BarcodePosition m) {
   switch (m) {
      case TUBE_TOPS: os << "top"; break;
      case TUBE_BOTTOMS: os << "bottom"; break;
      default:
         throw std::logic_error("invalid value for barcode position");
   }
   return os;
}

} /* namespace */
