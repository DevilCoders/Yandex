#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/system/types.h>

namespace NSaasTrie {
    extern const ui32 TrieVersion;
    extern const char TrieFileHeader[4];

    template<typename TKeyPrefix>
    inline TString GetTrieKey(TKeyPrefix&& kps, TStringBuf docUrl) {
        return ToString(kps) + '\t' + docUrl;
    }
}
