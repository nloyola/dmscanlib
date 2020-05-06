#ifndef __INCLUDE_IMAGE_SCANNER_H
#define __INCLUDE_IMAGE_SCANNER_H
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

#include <memory>
#include <opencv2/opencv.hpp>

#if defined (WIN32) && ! defined(__MINGW32__)
#   define NOMINMAX
#   include <windows.h>
#   include <twain.h>
#else
typedef void* HANDLE;
typedef void* DSMENTRYPROC;
#endif

namespace dmscanlib {

class Image;

/**
 * This class interfaces with the flatbed scanner to acquire images.
 */
class ImgScanner {
public:
   ImgScanner();

   virtual ~ImgScanner();

   static void setTwainDsmEntry(DSMENTRYPROC twainDsmEntry);

   /**
    * Factory method to create appropriate instance for operating system.
    */
   static std::unique_ptr<ImgScanner> create();

   virtual bool selectSourceAsDefault() = 0;

   virtual void getDeviceNames(std::vector<std::string> & names) = 0;

   virtual void selectDevice(const std::string & name) = 0;

   virtual void getValidDpis(std::vector<int> & validDpis) = 0;

   virtual int getScannerCapability() = 0;

   virtual void getFlatbedDimensionsInInches(std::pair<float, float> & dimensions) = 0;

   virtual std::unique_ptr<Image> acquireImage(unsigned dpi,
                                               int brightness,
                                               int contrast,
                                               float left,
                                               float top,
                                               float right,
                                               float bottom) = 0;

   virtual std::unique_ptr<Image> acquireFlatbed(unsigned dpi,
                                                 int brightness,
                                                 int contrast) = 0;

   virtual int getErrorCode() = 0;

   static const int MAX_BRIGHTNESS = 1000;

   static const int MIN_BRIGHTNESS = -1000;

   static const int MAX_CONTRAST = 1000;

   static const int MIN_CONTRAST = -1000;

protected:
};

} /* namespace */

/* Local Variables: */
/* mode: c++        */
/* End:             */

#endif /* __INCLUDE_IMAGE_SCANNER_H */
