#define _CRT_SECURE_NO_DEPRECATE

#include "ImgScanner.h"
#include "ImgScannerTwain.h"
#include "ImgScannerSane.h"

#include <memory>

namespace dmscanlib {

ImgScanner::ImgScanner() {}

ImgScanner::~ImgScanner() {}

std::unique_ptr<ImgScanner> ImgScanner::create() {
#ifdef WIN32
    return std::unique_ptr<ImgScanner>(new imgscanner::ImgScannerTwain());
#elif __GNUC__
    return std::unique_ptr<ImgScanner>(new imgscanner::ImgScannerSane());
#else
    #error No ImgScanner implementation for your operating system is available
#endif
}

} /* namespace */
