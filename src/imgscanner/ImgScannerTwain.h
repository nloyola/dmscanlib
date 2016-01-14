#ifndef __INCLUDE_IMG_SCANNER_IMPL_H
#define __INCLUDE_IMG_SCANNER_IMPL_H
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

#if defined (WIN32) && ! defined(__MINGW32__)

#include "imgscanner/ImgScanner.h"
#include "Image.h"
#include "DmScanLib.h"

#include <dmtx.h>
#include <twain.h>     // Standard TWAIN header.
#define NOMINMAX
#include <windows.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

namespace dmscanlib {

namespace imgscanner {

/**
 * Utility class to store a Range Capability's valid values.
 *
 * Meant for internal use only.
 */
class RangeCapability {
public:
   RangeCapability(double _min, double _max, double _step):
         min(_min), max(_max), step(_step) {}

   RangeCapability():
         min(-1), max(-1), step(-1) {}

   bool isValid(double value) {
      return ((value >= min) && (value <= max) && (std::fmod(value, step) == 0));
   }

   double min;
   double max;
   double step;
};

/**
 * Utility class to store a Capability's valid values.
 *
 * Meant for internal use only.
 */
class Capability {
public:
   Capability(double min, double max, double step): range(min, max, step) {}

   Capability() {}

   void init() {
      values.clear();
      range.min = -1;
      range.max = -1;
      range.step = -1;
   }

   void addValue(double value) {
      values.push_back(value);
   }

   void set(double min, double max, double step) {
      range.min = min;
      range.max = max;
      range.step = step;
   }

   bool isValid(double value) {
      if (!values.empty()) {
         return (std::find(values.begin(), values.end(), value) != values.end());
      } else {
         return ((value >= range.min)
                 && (value <= range.max)
                 && (std::fmod(value, range.step) == 0));
      }
      CHECK(FALSE) << "could not determine if capability is valid";
      return FALSE;
   }

   RangeCapability range;
   std::vector<double> values;
};

/**
 * This class interfaces with the TWAIN driver to acquire images from the
 * scanner.
 */
class ImgScannerTwain : public ImgScanner {
public:
   ImgScannerTwain();
   ~ImgScannerTwain();

   static void setTwainDsmEntry(DSMENTRYPROC twainDsmEntry);

   /**
    * returns false if user pressed cancel when presented with dialog box to
    * select a scanner.
    */
   bool selectSourceAsDefault();

   int getScannerCapability();

   void getDeviceNames(std::vector<std::string> & names);

   void selectDevice(const std::string & name);

   void getValidDpis(std::vector<int> & validDpis);

   void getFlatbedDimensionsInInches(std::pair<float, float> & dimensions);

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

   unsigned invokeTwain(TW_IDENTITY * srcId,
                        unsigned long dg,
                        unsigned dat,
                        unsigned msg,
                        void * data);

   void setFloatToIntPair(double f,
                          short & whole,
                          unsigned short & frac);

   //BOOL setCapability(TW_IDENTITY * srcId, TW_UINT16 cap,TW_UINT16 value,BOOL sign);

   BOOL setCapOneValue(TW_IDENTITY * srcId,
                       unsigned Cap,
                       unsigned ItemType,
                       unsigned long ItemVal);

   BOOL setCapOneValueFixedPoint(TW_IDENTITY * srcID,
                                 unsigned Cap,
                                 TW_INT16 whole,
                                 TW_UINT16 frac);

   double getCapabilityOneValue(TW_IDENTITY & srcID, TW_UINT16 cap);

   inline double uint32ToFloat(TW_UINT32 uint32);
   inline double twfix32ToFloat(TW_FIX32 fix32);

   void getCustomDsData(TW_IDENTITY * srcId);

   bool scannerSourceInit(HWND & hwnd, TW_IDENTITY & srcID);
   void scannerSourceDeinit(HWND & hwnd, TW_IDENTITY & srcID);

   int getScannerCapabilityInternal(TW_IDENTITY & srcID);
   void getCapabilityInternal(TW_IDENTITY & srcID,
                              TW_UINT16 cap,
                              Capability & capability);

   /**
    * g_pDSM_Entry holds the address of function DSM_Entry() in TWAIN_32.DLL. If
    * this address is 0, either TWAIN_32.DLL could not be loaded or there is no
    * DSM_Entry() function in TWAIN_32.DLL.
    */
   static DSMENTRYPROC twainDsmEntry;

   /**
    * g_AppID serves as a TWAIN identity structure that uniquely identifies the
    * application process responsible for making calls to function DSM_Entry().
    */
   static TW_IDENTITY twainAppID;

   Capability supportedDpis;

   int errorCode;
};

} /* namespace */

} /* namespace */

#endif /* defined (WIN32) && ! defined(__MINGW32__) */

/* Local Variables: */
/* mode: c++        */
/* End:             */

#endif /* __INCLUDE_IMG_SCANNER_IMPL_H */
