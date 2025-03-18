#pragma once

#include <nginx/modules/strm_packager/src/common/muxer.h>

#include <library/cpp/regex/pire/regexp.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NStrm::NPackager {
    class TMusicUri {
    public:
        enum EMusicContainer {
            Caf,
            Mp4,
        };

        explicit TMusicUri(TStringBuf uri);

        const TString GetDescriptionUri() const;
        bool IsValid(TRequestWorker& worker, const TString& secret) const;
        const TStringBuf& GetKeyIv() const;
        EMusicContainer GetContainer() const;

    private:
        static ui8 Unhex(ui8 c);
        static TString Unhex(const TStringBuf& c);

        TStringBuf Bucket;
        TStringBuf TrackId;
        TStringBuf MetaInfo;
        TStringBuf ValidUntil;
        TStringBuf KeyIv;
        TString Signature;
        EMusicContainer Container;
    };
}
