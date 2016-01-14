/*
 * TestCommon.cpp
 *
 *  Created on: 2012-11-05
 *      Author: nelson
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "test/TestCommon.h"
#include "DmScanLib.h"
#include "Image.h"
#include "decoder/WellRectangle.h"
#include "decoder/DecodeOptions.h"
#include "imgscanner/ImgScanner.h"

#include <sstream>

#include <algorithm>
#include <opencv/cv.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#ifdef _VISUALC_
#   pragma warning(disable : 4996)
#   define NOMINMAX
#   include <windows.h>
#   include <twain.h>
#else
#   include <dirent.h>
#endif

namespace dmscanlib {

namespace test {

#ifdef WIN32
static HMODULE g_hLib;
static DSMENTRYPROC g_pDSM_Entry;
#endif

void initializeTwain() {
#ifdef WIN32
	g_hLib = LoadLibraryA("TWAIN_32.DLL");

	// Report failure if TWAIN_32.DLL cannot be loaded and terminate
	CHECK(g_hLib != 0) << "Unable to open TWAIN_32.DLL";

	// Attempt to retrieve DSM_Entry() function address.
	g_pDSM_Entry =  (DSMENTRYPROC) GetProcAddress(g_hLib, "DSM_Entry");
	dmscanlib::ImgScanner::setTwainDsmEntry(g_pDSM_Entry);

	// Report failure if DSM_Entry() function not found in TWAIN_32.DLL
	// and terminate
	CHECK(g_pDSM_Entry != 0) "Unable to fetch DSM_Entry address";
#endif
}

// calling imgScanner::getDeviceNames() multiple times results in sometimes
// SANE not returning any device names.
//
// by implementing getFirstDevice() this way we avoid this problem.
static std::string firstDeviceName;

std::string & getFirstDevice() {
#ifndef WIN32
   if (firstDeviceName.empty()) {
      std::unique_ptr<ImgScanner> imgScanner = ImgScanner::create();
      std::vector<std::string> deviceNames;
      imgScanner->getDeviceNames(deviceNames);
      CHECK_GT(deviceNames.size(), 0);
      imgScanner->selectDevice(deviceNames[0]);
      firstDeviceName.append(deviceNames[0]);
   }
#endif

   return firstDeviceName;
}

/**
 * Do not care if call to delete file fails.
 */
void deleteFile(const std::string & filename) {
#ifdef WIN32
   std::wstring fnamew(filename.begin(), filename.end());
   DeleteFile(fnamew.c_str());
#else
   remove(filename.c_str());
#endif
}

/*
 * Gets file names for all the test information files in the "testImageInfo" folder.
 *
 * The test images the info files refer to can be downloaded from
 * http://aicml-med.cs.ualberta.ca/CBSR/scanlib/testImages.tar.bz2
 */
bool getTestImageInfoFilenames(std::string dir, std::vector<std::string> & filenames) {
#ifndef _VISUALC_
    DIR * dp;
    dirent * dirp;

    dp = opendir(dir.c_str());
    if (dp == NULL)
        return false;

    VLOG(3) << "getting files from directory: " << dir;

    while ((dirp = readdir(dp)) != NULL) {
        if (((dirp->d_type == DT_DIR) && (dirp->d_name[0] != '.'))) {
            std::string subdirname;
            subdirname.append(dir).append("/").append(dirp->d_name);
            getTestImageInfoFilenames(subdirname, filenames);
        } else if (dirp->d_type == DT_REG) {
            std::string basename(dirp->d_name);

            if (basename.find(".nfo") != std::string::npos) {
                filenames.push_back(std::string(dir).append("/").append(basename));
            }
        }
    }
    closedir(dp);
#else
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    //Specify a file mask. *.* = We want everything!
    std::wstring dirw;
    dirw.assign(dir.begin(), dir.end());

    std::wstring searchstrw(dirw);
    searchstrw.append(L"\\*.*");

    if((hFind = FindFirstFile((LPCWSTR) searchstrw.c_str(), &fdFile)) == INVALID_HANDLE_VALUE) {
        //VLOG(1) << "error is: " << GetLastError();
        return false;
    }

    do {
        //Find first file will always return "."
        //    and ".." as the first two directories.
        if(fdFile.cFileName[0] != '.') {

            //Is the entity a File or Folder?
            if(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring subdirnamew(dirw);
                subdirnamew.append(L"\\").append((wchar_t *)fdFile.cFileName);

                std::string subdirname;
                subdirname.assign(subdirnamew.begin(), subdirnamew.end());
                getTestImageInfoFilenames(subdirname, filenames);
            } else {
                std::wstring basenamew((wchar_t *)fdFile.cFileName);
                std::string basename;
                basename.assign(basenamew.begin(), basenamew.end());

                if (basename.find(".nfo") != std::string::npos) {
                    filenames.push_back(std::string(dir).append("\\").append(basename));
                }
            }
        }
    }
    while(FindNextFile(hFind, &fdFile));

    FindClose(hFind);
#endif
    return true;
}

void getWellRectsForBoundingBox(const cv::Rect & bbox,
                                unsigned rows,
                                unsigned cols,
                                Orientation orientation,
                                BarcodePosition position,
                                std::vector<std::unique_ptr<const WellRectangle> > & wellRects) {

    float wellWidth = bbox.width / static_cast<float>(cols);
    float wellHeight = bbox.height / static_cast<float>(rows);

    cv::Point2f horTranslation(wellWidth, 0);
    cv::Point2f verTranslation(0, wellHeight);

    cv::Size cellSize(static_cast<unsigned>(wellWidth),
                      static_cast<unsigned>(wellHeight));

    float horOffset;
    float verOffset = static_cast<float>(bbox.y);

    for (unsigned row = 0; row < rows; ++row) {
        horOffset = static_cast<float>(bbox.x);

        for (unsigned col = 0; col < cols; ++col) {
            std::string label = DmScanLib::getLabelForPosition(row,
                                                               col,
                                                               rows,
                                                               cols,
                                                               orientation,
                                                               position);

            std::unique_ptr<WellRectangle> wellRect(
                    new WellRectangle(
                            label.c_str(),
                            static_cast<unsigned>(horOffset),
                            static_cast<unsigned>(verOffset),
                            cellSize.width,
                            cellSize.height));
            const cv::Rect & wRect = wellRect->getRectangle();

            if (!bbox.contains(wRect.tl())) {
               LOG(WARNING) << "well rectangle top-left outside bounding box: "
                            << label
                            << ", " << wRect.tl();
               wellRects.clear();
               return;
            }

            if (!bbox.contains(wRect.br())) {
               LOG(WARNING) << "well rectangle bottom-right outside bounding box: "
                            << label
                            << ", " << wRect.br();
               wellRects.clear();
               return;
            }

            VLOG(5) << "getWellRectsForBoundingBox: " << *wellRect;
            wellRects.push_back(std::move(wellRect));

            horOffset += wellWidth;
        }
        verOffset += wellHeight;
    }
}

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions() {
    const double minEdgeFactor = 0.15;
    const double maxEdgeFactor = 0.3;
    const double scanGapFactor = 0.1;
    const long squareDev = 10;
    const long edgeThresh = 5;
    const long corrections = 10;
    const long shrink = 1;

    return std::unique_ptr<DecodeOptions>(new DecodeOptions(
            minEdgeFactor, maxEdgeFactor, scanGapFactor, squareDev, edgeThresh, corrections, shrink));
}

int decodeImage(std::string fname, DmScanLib & dmScanLib, unsigned rows, unsigned cols) {
    std::vector<std::unique_ptr<const WellRectangle> > wellRects;

    Image image(fname);
    CHECK(image.isValid()) << "could not load image";

    cv::Size size = image.size();
    cv::Rect bbox(0, 0, size.width, size.height);

    getWellRectsForBoundingBox(bbox, rows, cols, LANDSCAPE, TUBE_BOTTOMS, wellRects);

    std::unique_ptr<DecodeOptions> decodeOptions = getDefaultDecodeOptions();
    return dmScanLib.decodeImageWells(fname.c_str(), *decodeOptions, wellRects);
}

} /* namespace test */

} /* namespace dmscanlib */
