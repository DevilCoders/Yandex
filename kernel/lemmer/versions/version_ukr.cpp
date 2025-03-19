#include "versions.h"
#include <library/cpp/archive/yarchive.h>
#include <library/cpp/langs/langs.h>
#include <util/stream/file.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

namespace {
    extern "C" {
    extern const unsigned char VersionUkrBinTrieData[];
    extern const ui32 VersionUkrBinTrieDataSize;
    };

    class TVersionUkr {
    public:
        TVersionUkr() {
            TArchiveReader reader(TBlob::NoCopy(VersionUkrBinTrieData, VersionUkrBinTrieDataSize));
            TAutoPtr<IInputStream> src(reader.ObjectByKey("/version_ukr.trie").Release());
            NLanguageVersioning::InitializeHash(*src, LANG_UKR);
        }
    };

    auto VersionUkr = Singleton<TVersionUkr>();
}

void UseVersionUkr() {
    // spike for -flto build (SEARCH-1154)
    Y_UNUSED(VersionUkr);
}
