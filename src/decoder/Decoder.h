#ifndef DECODER_H_
#define DECODER_H_

/*
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 * Copyright (C) 2010 Canadian Biosample Repository
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

#include "Image.h"
#include "WellRectangle.h"

#include <dmtx.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

#ifdef WIN32
#   define NOMINMAX
#   include <windows.h>
#endif

namespace dmscanlib {

class DecodeOptions;
class WellDecoder;

namespace decoder {
class DmtxDecodeHelper;
}

class Decoder {
public:
    Decoder(const Image & image, const DecodeOptions & decodeOptions,
            std::vector<std::unique_ptr<const WellRectangle> > & wellRects);
    virtual ~Decoder();
    int decodeWellRects();
    void decodeWellRect(const Image & wellRectImage, WellDecoder & wellDecoder) const;

    const Image & getWorkingImage() const {
        return grayscaleImage;
    }

    const unsigned getDecodedWellCount();

    std::vector<std::unique_ptr<WellDecoder> > & getWellDecoders() {
        return wellDecoders;
    }

    const std::map<std::string, const WellDecoder *> & getDecodedWells() const;

    static void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);

    static void writeDiagnosticImage(DmtxDecode *dec, const std::string & id);

private:
    void applyFilters();
    void decodeWellRect(WellDecoder & wellDecoder, DmtxDecode *dec) const;
    std::unique_ptr<decoder::DmtxDecodeHelper> createDmtxDecode(
            DmtxImage * dmtxImage,
            WellDecoder & wellDecoder,
            int scale) const;

    void getDecodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg,
            WellDecoder & wellDecoder) const;

    int decodeSingleThreaded();
    int decodeMultiThreaded();

    Image grayscaleImage;
    const DecodeOptions & decodeOptions;
    const std::vector<std::unique_ptr<const WellRectangle> > & wellRects;
    std::vector<std::unique_ptr<WellDecoder> > wellDecoders;
    bool decodeSuccessful;
    std::map<std::string, const WellDecoder *> decodedWells;
};

} /* namespace */

#endif /* DECODER_H_ */
