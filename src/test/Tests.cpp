/**
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

#define _CRT_SECURE_NO_DEPRECATE

#include "DmScanLib.h"
#include "test/TestCommon.h"

#include <gtest/gtest.h>
#include <getopt.h>

int getLogLevelFromCommandLine() {
    int logLevel = 0;
    std::string verbosePrefix("--v=");

    std::vector<std::string> args = ::testing::internal::GetInjectableArgvs();
    for(std::vector<std::string>::iterator it = args.begin(); it != args.end(); ++it) {
        if (!it->compare(0, verbosePrefix.size(), verbosePrefix)) {
            logLevel = atoi(it->substr(verbosePrefix.size()).c_str());
        }
    }

    return logLevel;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    dmscanlib::DmScanLib::configLogging(getLogLevelFromCommandLine(), false);

    dmscanlib::test::getFirstDevice();
    dmscanlib::test::initializeTwain();
    int result = RUN_ALL_TESTS();

    // uncomment next line to wait for user to press enter key
    //std::getchar();

    return result;
}
