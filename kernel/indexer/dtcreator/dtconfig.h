#pragma once
#include <util/generic/string.h>

namespace NIndexerCore {

struct TDTCreatorConfig {
    TString PureTrieFile;
    TString PureLangConfigFile;
    size_t CacheHashSize;
    size_t CacheBlockSize;
    bool KiwiTrigger; // TDirectTextCreator works with serialized numerator events in a kiwi trigger

    TDTCreatorConfig(size_t cacheHashSize = 49000, size_t cacheBlockSize = 10 << 20)
        : CacheHashSize(cacheHashSize)
        , CacheBlockSize(cacheBlockSize)
        , KiwiTrigger(false)
    {}
};

}
