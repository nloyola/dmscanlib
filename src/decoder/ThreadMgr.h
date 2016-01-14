#ifndef __INC_PROCESS_IMAGE_MANAGER_H
#define __INC_PROCESS_IMAGE_MANAGER_H

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

#include <dmtx.h>
#include <string>
#include <memory>
#include <vector>

#ifdef _VISUALC_
#   include <functional>
#endif

namespace dmscanlib {

class WellDecoder;

namespace decoder {

class ThreadMgr {
public:
    ThreadMgr();
    ~ThreadMgr();

    void decodeWells(std::vector<std::unique_ptr<dmscanlib::WellDecoder> > & wellDecoders);

private:
    static const unsigned THREAD_NUM;

    void threadHandler();
    void threadProcessRange(unsigned int first, unsigned int last);

    std::vector<dmscanlib::WellDecoder *> allThreads;
    unsigned numThreads;
};

} /* namespace */

} /* namespace */

#endif /* __INC_PROCESS_IMAGE_MANAGER_H */
