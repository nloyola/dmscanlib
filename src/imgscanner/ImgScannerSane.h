#ifndef __INCLUDE_IMG_SCANNER_SANE_H
#define __INCLUDE_IMG_SCANNER_SANE_H

/*
  Dmscanlib is a software library and standalone application that scans
  and decodes libdmtx compatible test-tubes. It is currently designed
  to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
  Copyright (C) 2010 Canadian Biosample Repository

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "imgscanner/ImgScanner.h"
#include "imgscanner/SaneOption.h"

#include <sane/sane.h>
#include <vector>
#include <utility>
#include <memory>

namespace dmscanlib {

namespace imgscanner {

/**
 * This class interfaces with the TWAIN driver to acquire images from the
 * scanner.
 */
class ImgScannerSane: public ImgScanner {
public:
   ImgScannerSane();
   virtual ~ImgScannerSane();

   /**
    * In Linux, use getDeviceNames() and selectDevice() to select a device.
    */
   bool selectSourceAsDefault();

   void getDeviceNames(std::vector<std::string> & names);

   void selectDevice(std::string const & name);

   int getScannerCapability();

   void getFlatbedDimensionsInInches(std::pair<double, double> & pair);

   void getBrightnessRange(std::pair<double, double> & pair);

   void getContrastRange(std::pair<double, double> & pair);

   /**
    * bbox units are in inches.
    */
   std::unique_ptr<Image> acquireImage(const unsigned dpi,
                                       const int brightness,
                                       const int contrast,
                                       const cv::Rect_<float> & bboxInches);

   std::unique_ptr<Image>  acquireFlatbed(const unsigned dpi,
                                          const int brightness,
                                          const int contrast);

   int getErrorCode() { return errorCode; }

private:
   void init();

   SaneOptionRangeConstraint<double> const * getOptionRangeDouble(int optionNumber);

   SaneOptionRangeConstraint<int> const * getOptionRangeInt(int optionNumber);

   SaneOptionConstraint<int> const * getOptionConstraintInt(int optionNumber);

   std::unique_ptr<Image> acquireImageInternal(const unsigned dpi,
                                               const int brightness,
                                               const int contrast,
                                               const cv::Rect_<double> & bboxMillimeters);

   static double inchesToMm;
   static double mmToInches;

   std::string deviceName;
   SANE_Handle saneHandle;

   std::map<int, std::unique_ptr<const SaneOption>> saneOptions;

   int errorCode;

   int resolutionOption;
   int scanModeOption;
   int contrastOption;
   int brightnessOption;
   int compressionOption;
   int geometryLeftOption;
   int geometryTopOption;
   int geometryRightOption;
   int geometryBottomOption;
};

} /* namespace */

} /* namespace */

/* Local Variables: */
/* mode: c++        */
/* End:             */

#endif /* __INCLUDE_IMG_SCANNER_SANE_H */
