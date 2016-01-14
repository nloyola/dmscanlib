#ifndef __INCLUDE_IMG_SCANNER_SANE_H
#define __INCLUDE_IMG_SCANNER_SANE_H

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

   void selectDevice(const std::string & name);

   int getScannerCapability();

   void getValidDpis(std::vector<int> & validDpis);

   void getFlatbedDimensionsInInches(std::pair<float, float> & dimensions);

   void getBrightnessRange(std::pair<int, int> & pair);

   void getContrastRange(std::pair<int, int> & pair);

   /**
    * Flatbed coordinates are in inches.
    */
   std::unique_ptr<Image> acquireImage(unsigned dpi,
                                       int brightness,
                                       int contrast,
                                       float left,
                                       float top,
                                       float right,
                                       float bottom);

   std::unique_ptr<Image>  acquireFlatbed(unsigned dpi,
                                          int brightness,
                                          int contrast);

   int getErrorCode() { return errorCode; }

private:
   void init();

   SaneOptionRangeConstraint<double> const * getOptionRangeDouble(int optionNumber);

   SaneOptionRangeConstraint<int> const * getOptionRangeInt(int optionNumber);

   SaneOptionConstraint<int> const * getOptionConstraintInt(int optionNumber);

   std::unique_ptr<Image> acquireImageInternal(unsigned dpi,
                                               int brightness,
                                               int contrast,
                                               float left,
                                               float top,
                                               float right,
                                               float bottom);

   static const double inchesToMm;
   static const double mmToInches;

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
