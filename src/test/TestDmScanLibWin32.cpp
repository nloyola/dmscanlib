/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "test/TestCommon.h"
#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"
#include "decoder/WellDecoder.h"
#include "Image.h"

#include <stdexcept>
#include <stddef.h>
#include <sys/types.h>

#ifdef _VISUALC_
#   pragma warning(disable : 4996)
#else
#   include <dirent.h>
#endif

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

TEST(TestDmScanLibWin32, selectSourceAsDefault) {
   DmScanLib dmScanLib(1);
   ASSERT_EQ(SC_SUCCESS, dmScanLib.selectSourceAsDefault());
}

TEST(TestDmScanLibWin32, getScannerCapability) {
   DmScanLib dmScanLib(1);
   int result = dmScanLib.getScannerCapability();

   ASSERT_EQ(CAP_IS_WIA, result & CAP_IS_WIA);
   ASSERT_EQ(CAP_DPI_300, result & CAP_DPI_300);
   ASSERT_EQ(CAP_DPI_400, result & CAP_DPI_400);
   ASSERT_EQ(CAP_DPI_600, result & CAP_DPI_600);
}

} /* namespace */
