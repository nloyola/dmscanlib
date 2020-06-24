/**
 * An ImgScanner implementation using the SANE (Scanner Access Now Easy) backend.
 *
 * See: http://anonscm.debian.org/cgit/sane/sane-backends.git/tree/frontend/scanimage.c
 *
 *---
 *
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

#include "imgscanner/ImgScannerSane.h"
#include "imgscanner/SaneOption.h"
#include "imgscanner/SaneUtil.h"
#include "Image.h"
#include "DmScanLib.h"

#include <glog/logging.h>
#include <math.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace dmscanlib {

namespace imgscanner {

const double ImgScannerSane::inchesToMm = 25.4;
const double ImgScannerSane::mmToInches = 1.0 / ImgScannerSane::inchesToMm;

ImgScannerSane::ImgScannerSane() {
    init();
}

ImgScannerSane::~ImgScannerSane() {
}

void ImgScannerSane::init() {
    VLOG(5) << "init:";

    deviceName.clear();
    saneOptions.clear();

    resolutionOption     = -1;
    scanModeOption       = -1;
    contrastOption       = -1;
    brightnessOption     = -1;
    compressionOption    = -1;
    geometryLeftOption   = -1;
    geometryTopOption    = -1;
    geometryRightOption  = -1;
    geometryBottomOption = -1;
}

SaneOptionRangeConstraint<double> const *
ImgScannerSane::getOptionRangeDouble(int optionNumber) {
    SaneOption const * option = saneOptions[optionNumber].get();
    SaneOptionRangeConstraint<double> const * rangeOption =
        dynamic_cast<SaneOptionRangeConstraint<double> const *>(option);
    if (rangeOption == NULL) {
        CHECK(false) << "dynamic cast failed for option: " << optionNumber;
        return NULL;
    }
    return rangeOption;
}

SaneOptionRangeConstraint<int> const *
ImgScannerSane::getOptionRangeInt(int optionNumber) {
    SaneOption const * option = saneOptions[optionNumber].get();
    SaneOptionRangeConstraint<int> const * rangeOption =
        dynamic_cast<SaneOptionRangeConstraint<int> const *>(option);
    if (rangeOption == NULL) {
        CHECK(false) << "dynamic cast failed for option: " << optionNumber;
        return NULL;
    }
    return rangeOption;
}

SaneOptionConstraint<int> const *
ImgScannerSane::getOptionConstraintInt(int optionNumber) {
    SaneOption const * option = saneOptions[optionNumber].get();
    SaneOptionConstraint<int> const * constraintOption =
        dynamic_cast<SaneOptionConstraint<int> const *>(option);
    if (constraintOption == NULL) {
        CHECK(false) << "dynamic cast failed for option: " << optionNumber;
        return NULL;
    }
    return constraintOption;
}

bool ImgScannerSane::selectSourceAsDefault() {
    return false;
}

void ImgScannerSane::getDeviceNames(std::vector<std::string> & names) {
    names.clear();
    const SANE_Device ** deviceListPtr;
    SANE_Status status = sane_init(NULL, NULL);
    CHECK_EQ(status, SANE_STATUS_GOOD) << "SANE initialization failed";

    status = sane_get_devices(&deviceListPtr, SANE_TRUE);
    CHECK_EQ(status, SANE_STATUS_GOOD) << "SANE get devices failed";

    unsigned i = 0;
    const SANE_Device * deviceList = deviceListPtr[i];

    while (deviceList != NULL) {
        names.push_back(deviceList->name);
        VLOG(3) << "name: " << deviceList->name
                << ", vendor: " << deviceList->vendor
                << ", model: " << deviceList->model
                << ", type: " << deviceList->type;
        ++i;
        deviceList = deviceListPtr[i];
    }
    sane_exit();
}

void ImgScannerSane::selectDevice(const std::string & name) {
    if (deviceName.compare(name) != 0) {
        init();
        deviceName.append(name);
    }
}

void ImgScannerSane::getValidDpis(std::vector<int> & validDpis) {
    VLOG(1) << "getValidDpis";

    CHECK_GT(deviceName.length(), 0) << "no device selected";

    if (saneOptions.empty()) {
        getScannerCapability();
    }

    validDpis.clear();
    const SaneOptionConstraint<int> * resolutionOpt = getOptionConstraintInt(resolutionOption);
    resolutionOpt->copyItems(validDpis);
}

int ImgScannerSane::getScannerCapability() {
    VLOG(2) << "getScannerCapability";

    CHECK_GT(deviceName.length(), 0) << "no device selected";

    saneHandle = SaneUtil::openDevice(deviceName);

    SANE_Int numOptions = SaneUtil::getControlOptionWord(saneHandle, 0);
    VLOG(5) << "num options: " << numOptions;

    const SaneOptionGroup * currentGroup = NULL;

    for (int i = 0; i < numOptions; ++i) {
        saneOptions.insert(make_pair(i, SaneOptionFactory::createOption(saneHandle, i, currentGroup)));

        if (saneOptions[i]->getType() == SANE_TYPE_GROUP) {
            currentGroup = static_cast<const SaneOptionGroup *>(saneOptions[i].get());
        }
    }

    for (auto & kv : saneOptions) {
        SaneOption const & option = *kv.second;

        resolutionOption = (option.getName().compare("resolution") == 0)
            ? option.getOptionNumber() : resolutionOption;
        compressionOption = (option.getName().compare("compression") == 0)
            ? option.getOptionNumber() : compressionOption;
        scanModeOption = (option.getName().compare("mode") == 0)
            ? option.getOptionNumber() : scanModeOption;

        brightnessOption = (option.getName().compare("brightness") == 0)
            ? option.getOptionNumber() : brightnessOption;
        contrastOption = (option.getName().compare("contrast") == 0)
            ? option.getOptionNumber() : contrastOption;

        geometryLeftOption = (option.getName().compare("tl-x") == 0)
            ? option.getOptionNumber() : geometryLeftOption;
        geometryTopOption = (option.getName().compare("tl-y") == 0)
            ? option.getOptionNumber() : geometryTopOption;
        geometryRightOption = (option.getName().compare("br-x") == 0)
            ? option.getOptionNumber() : geometryRightOption;
        geometryBottomOption = (option.getName().compare("br-y") == 0)
            ? option.getOptionNumber() : geometryBottomOption;

        if (option.getType() == SANE_TYPE_GROUP) {
            VLOG(5) << option;
        } else {
            VLOG(5) << option << "," << std::endl << "\tvalue: " << option.valueAsString(saneHandle);
        }
    }

    sane_close(saneHandle);
    sane_exit();

    return 1;
}

void ImgScannerSane::getFlatbedDimensionsInInches(std::pair<float, float> & dimensions) {
    VLOG(2) << "getFlatbedDimensionsInInches";

    CHECK_GT(deviceName.length(), 0) << "no device selected";

    if (saneOptions.empty()) {
        getScannerCapability();
    }

    SaneOptionRangeConstraint<double> const * rightOpt = getOptionRangeDouble(geometryRightOption);
    SaneOptionRangeConstraint<double> const * bottomOpt = getOptionRangeDouble(geometryBottomOption);

    dimensions.first = rightOpt->getMax() * mmToInches;
    dimensions.second = bottomOpt->getMax() * mmToInches;
}

/**
 * bbox units are in inches.
 */
std::unique_ptr<Image> ImgScannerSane::acquireImage(unsigned dpi,
                                                    int brightness,
                                                    int contrast,
                                                    float left,
                                                    float top,
                                                    float right,
                                                    float bottom) {
    VLOG(3) << "acquireImage: source: \"" << deviceName << "\""
            << ", dpi: " << dpi
            << ", brightness:" << brightness
            << ", constrast:" << contrast
            << ", left: " << left
            << ", top: " << top
            << ", right: " << right
            << ", bottom: " << bottom;

    CHECK_GT(deviceName.length(), 0) << "no device selected";

    double leftMillimeters   = static_cast<double>(left) * inchesToMm;
    double topMillimeters    = static_cast<double>(top) * inchesToMm;
    double rightMillimeters  = static_cast<double>(right) * inchesToMm;
    double bottomMillimeters = static_cast<double>(bottom) * inchesToMm;

    return acquireImageInternal(dpi,
                                brightness,
                                contrast,
                                leftMillimeters,
                                topMillimeters,
                                rightMillimeters,
                                bottomMillimeters);
}

std::unique_ptr<Image>  ImgScannerSane::acquireFlatbed(unsigned dpi, int brightness, int contrast) {
    VLOG(2) << "acquireFlatbed: source: \"" << deviceName << "\""
            << ", dpi: " << dpi
            << ", brightness:" << brightness
            << ", constrast:" << contrast;

    if (saneOptions.empty()) {
        getScannerCapability();
    }

    double left = getOptionRangeDouble(geometryLeftOption)->getMin();
    double top = getOptionRangeDouble(geometryTopOption)->getMin();
    double right = getOptionRangeDouble(geometryRightOption)->getMax();
    double bottom = getOptionRangeDouble(geometryBottomOption)->getMax();

    cv::Rect_<double> scanRect(left, top, right, bottom);
    return acquireImageInternal(dpi, brightness, contrast, left, top, right, bottom);
}

/**
 * Flatbed coordinates are in inches.
 */
std::unique_ptr<Image> ImgScannerSane::acquireImageInternal(unsigned dpi,
                                                            int brightness,
                                                            int contrast,
                                                            float left,
                                                            float top,
                                                            float right,
                                                            float bottom) {
    VLOG(2) << "acquireImageInternal: source: \"" << deviceName << "\""
            << ", dpi: " << dpi
            << ", brightness:" << brightness
            << ", constrast:" << contrast
            << ", left: " << left
            << ", top: " << top
            << ", right: " << right
            << ", bottom: " << bottom;

    if (saneOptions.empty()) {
        getScannerCapability();
    }

    SaneOptionConstraint<int> const * resolutionOpt = getOptionConstraintInt(resolutionOption);

    if (!resolutionOpt->contains(dpi)) {
        errorCode = SC_INVALID_DPI;
        return std::unique_ptr<Image>{};
    }

    if ((brightness < MIN_BRIGHTNESS) || (brightness > MAX_BRIGHTNESS )) {
        errorCode = SC_INVALID_BRIGHTNESS;
        return std::unique_ptr<Image>{};
    }

    if ((contrast < MIN_CONTRAST) || (contrast > MAX_CONTRAST )) {
        errorCode = SC_INVALID_CONTRAST;
        return std::unique_ptr<Image>{};
    }

    const SaneOptionRangeConstraint<double> * rightOpt = getOptionRangeDouble(geometryRightOption);
    const SaneOptionRangeConstraint<double> * bottomOpt = getOptionRangeDouble(geometryBottomOption);

    CHECK(rightOpt->validValue(left)) << "bounding box exeeds flatbed dimensions";
    CHECK(rightOpt->validValue(right)) << "bounding box exeeds flatbed dimensions";

    CHECK(bottomOpt->validValue(top)) << "bounding box exeeds flatbed dimensions";
    CHECK(bottomOpt->validValue(bottom)) << "bounding box exeeds flatbed dimensions";

    CHECK_GT(right, left) << "bounding box exeeds flatbed dimensions";
    CHECK_GT(bottom, top) << "bounding box exeeds flatbed dimensions";

    errorCode = SC_SUCCESS;

    saneHandle = SaneUtil::openDevice(deviceName);

    // configure device for scan
    const SaneOptionRangeConstraint<int> * brightnessOpt = getOptionRangeInt(brightnessOption);
    const SaneOptionRangeConstraint<int> * contrastOpt = getOptionRangeInt(contrastOption);

    int scaledBrightness = SaneUtil::scaleValue(brightness, brightnessOpt->getMax(), 1000);
    int scaledContrast = SaneUtil::scaleValue(contrast, contrastOpt->getMax(), 1000);

    SaneUtil::setControlOptionWord(saneHandle, resolutionOption, dpi);
    SaneUtil::setControlOptionString(saneHandle, scanModeOption, (SANE_String) "Color");
    SaneUtil::setControlOptionString(saneHandle, compressionOption, (SANE_String) "None");
    SaneUtil::setControlOptionWord(saneHandle, brightnessOption, scaledBrightness);
    SaneUtil::setControlOptionWord(saneHandle, contrastOption, scaledContrast);
    SaneUtil::setControlOptionWord(saneHandle, geometryLeftOption, SANE_FIX(left));
    SaneUtil::setControlOptionWord(saneHandle, geometryTopOption, SANE_FIX(top));
    SaneUtil::setControlOptionWord(saneHandle, geometryRightOption, SANE_FIX(right));
    SaneUtil::setControlOptionWord(saneHandle, geometryBottomOption, SANE_FIX(bottom));

    VLOG(5) << "scan region: ("
            << SANE_UNFIX(SaneUtil::getControlOptionWord(saneHandle, geometryLeftOption))
            << ", "
            << SANE_UNFIX(SaneUtil::getControlOptionWord(saneHandle, geometryTopOption))
            << "), ("
            << SANE_UNFIX(SaneUtil::getControlOptionWord(saneHandle, geometryRightOption))
            << ", "
            << SANE_UNFIX(SaneUtil::getControlOptionWord(saneHandle, geometryBottomOption))
            << ")";

    SANE_Status status = sane_start(saneHandle);
    CHECK_EQ(status, SANE_STATUS_GOOD) << "SANE start failed" << status;

    SANE_Parameters saneParams;
    status = sane_get_parameters(saneHandle, &saneParams);
    CHECK_EQ(status, SANE_STATUS_GOOD) << "SANE get parameters failed" << status;

    VLOG(3) << "format: " << saneParams.format
            << ", last_frame: " << saneParams.last_frame
            << ", lines: " << saneParams.lines
            << ", depth: " << saneParams.depth
            << ", pixels_per_line: " << saneParams.pixels_per_line
            << ", bytes_per_line: " << saneParams.bytes_per_line;

    CHECK_EQ(saneParams.format, SANE_FRAME_RGB) << "flatbed configuration invalid format";
    CHECK_EQ(saneParams.depth, 8) << "flatbed configuration invalid depth";

    SANE_Int len;
    unsigned imageSizeBytes = saneParams.bytes_per_line * saneParams.lines;
    unsigned bufRemaining = imageSizeBytes;

    cv::Mat imageMat(saneParams.lines, saneParams.pixels_per_line, CV_8UC3);
    unsigned char* bufptr = static_cast<unsigned char *>(imageMat.ptr());

    status = sane_read(saneHandle, (SANE_Byte*) bufptr, bufRemaining, &len);
    while (status != SANE_STATUS_EOF) {
        if (len) {
            bufptr = static_cast<unsigned char *>(bufptr + len);
            bufRemaining -= len;
        }
        status = sane_read(saneHandle, (SANE_Byte*) bufptr, bufRemaining, &len);
    }

    sane_cancel(saneHandle);
    SaneUtil::closeDevice(saneHandle);

    cv::cvtColor(imageMat, imageMat, cv::COLOR_RGB2BGR); // OpenCV default is BGR
    return std::unique_ptr<Image>(new Image(imageMat));
}

} /* namespace imgscanner */

} /* namespace dmscanlib*/