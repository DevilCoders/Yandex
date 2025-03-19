#pragma once

#include "trie_data.h"

#include <util/generic/string.h>

#include <utility>

namespace NDistBetweenWords {


    class TLevensteinWordDist {
    public:
        static const wchar16 BIGRAM_DELIMITER;

        TLevensteinWordDist(const TWordDistTriePtr& trie, TTrieData::TGetter getter):
                Trie(trie),
                Getter(std::move(getter))
        {}

        float ReplaceWeight(const TUtf16String& str1, const TUtf16String& str2) const;
        float InsertWeight(const TUtf16String& str) const;
        float DeleteWeight(const TUtf16String& str) const;

    private:
        TWordDistTriePtr Trie;
        TTrieData::TGetter Getter;

        static const TTrieData UNKNOWN_DATA;
        static const TTrieData UNKNOWN_BIGRAM_DATA;
    };

}
