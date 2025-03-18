#pragma once

#include <tools/snipmake/common/common.h>
#include <util/stream/zlib.h>

namespace NSnippets {

    class TSerpXmlReader {
    private:
        struct TParser;

        TBufferedZLibDecompress Data;
        THolder<TParser> Parser;
        bool Eof;
    public:
        TSerpXmlReader(IInputStream* serp);
        ~TSerpXmlReader();

        bool Next();
        const TReqSerp& Get() const;
    };

}
