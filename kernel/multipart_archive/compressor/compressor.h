#pragma once

#include <kernel/multipart_archive/abstract/part.h>

namespace NRTYArchive {

    class TCompressor {
    public:
        TCompressor(const IArchivePart::TConstructContext& ctx);
        IOutputStream* CreateCompressStream(IOutputStream& slave, size_t dataSize) const;
        IInputStream* CreateDecompressStream(IInputStream& slave) const;

    private:
        IArchivePart::TConstructContext::TCompressionParams Config;
    };

}
