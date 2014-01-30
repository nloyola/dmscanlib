/*
 * TestDmScanLibLinux.cpp
 *
 *  Created on: 2014-01-30
 *      Author: nelson
 */

#include "imgscanner/ImgScannerSane.h"
#include "DmScanLib.h"
#include "test/TestCommon.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

TEST(TestDmScanLibLinux, selectSourceAsDefault) {
    FLAGS_v = 3;
    DmScanLib dmScanLib(1);
    ASSERT_EQ(SC_SUCCESS, dmScanLib.selectSourceAsDefault());
}

} /* namespace */
