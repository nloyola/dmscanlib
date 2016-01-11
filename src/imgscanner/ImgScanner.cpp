#define _CRT_SECURE_NO_DEPRECATE

#include "ImgScanner.h"

#ifdef WIN32
#   include "ImgScannerTwain.h"
#else
#   include "ImgScannerSane.h"
#endif

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

void ImgScanner::setTwainDsmEntry(DSMENTRYPROC twainDsmEntry) {
   imgscanner::ImgScannerTwain::setTwainDsmEntry(twainDsmEntry);
}



} /* namespace */
