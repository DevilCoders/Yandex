#pragma once

#include <yweb/robot/logel/logelrecords.h>

#include <util/generic/string.h>

namespace NSnippets {

    class TZoraThumb {
    public:
        TString ImageCRC;
        TString Thumbnail;
        TString ImageInfoBlob; // cv/parserlib/genericimageattrs.proto
        MimeTypes MimeType;
    };

    void ParseThumb(TZoraThumb& res, const TLogel<TUnknownRec>* logel);
    void ParseThumbFromOrigDoc(TZoraThumb& res, const TLogel<TUnknownRec>* logel);
}
