#ifndef __INC_TEST_COMMON
#define __INC_TEST_COMMON

/*
 * TestCommon.h
 *
 *  Created on: 2012-11-05
 *      Author: nelson
 */

#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"
#include "DmScanLib.h"

#include <string>
#include <vector>
#include <memory>

namespace dmscanlib {

class DmScanLib;

namespace test {

void initializeTwain();

std::string & getFirstDevice();

void deleteFile(const std::string & filename);

bool getTestImageInfoFilenames(std::string dir, std::vector<std::string> & filenames);

void getWellRectsForBoundingBox(const cv::Rect & bbox,
                                unsigned rows,
                                unsigned cols,
                                Orientation orientation,
                                BarcodePosition position,
                                std::vector<std::unique_ptr<const WellRectangle> > & wellRects);

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions();

int decodeImage(std::string fname, DmScanLib & dmScanLib, unsigned rows, unsigned cols);

const int IMAGE_PIXELS_THRESHOLD = 5;

} /* namespace */

} /* namespace */

/* Local Variables: */
/* mode: c++        */
/* End:             */

#endif /* __INC_TEST_COMMON */
