#include "levenstein_word_dist.h"

#include "levenstein_dist_with_bigrams.h"

#include <library/cpp/charset/wide.h>

namespace NDistBetweenWords {
    const TTrieData TLevensteinWordDist::UNKNOWN_DATA = TTrieData{1.0f, 1.0f, 1.0f, 1.0f};
    const TTrieData TLevensteinWordDist::UNKNOWN_BIGRAM_DATA = TTrieData{
        NLevenstein::FORBIDDEN_BIGRAM_WEIGHT,
        NLevenstein::FORBIDDEN_BIGRAM_WEIGHT,
        NLevenstein::FORBIDDEN_BIGRAM_WEIGHT,
        NLevenstein::FORBIDDEN_BIGRAM_WEIGHT
    };

    const wchar16 TLevensteinWordDist::BIGRAM_DELIMITER = ' ';

    float TLevensteinWordDist::ReplaceWeight(const TUtf16String& str1, const TUtf16String& str2) const {
        if (Trie) {
            const TUtf16String* str1Ptr = &str1;
            const TUtf16String* str2Ptr = &str2;
            if (str1 > str2) {
                DoSwap(str1Ptr, str2Ptr);
            }
            auto key = BuildKey(*str1Ptr, *str2Ptr);

            const TTrieData* unknownData = &UNKNOWN_DATA;
            if (str1.find(BIGRAM_DELIMITER) != TUtf16String::npos || str2.find(BIGRAM_DELIMITER) != TUtf16String::npos) {
                unknownData = &UNKNOWN_BIGRAM_DATA;
            }

            return Getter(Trie->GetDefault(key, *unknownData));
        } else {
            return 0;
        }
    }

    float TLevensteinWordDist::InsertWeight(const TUtf16String& str) const {
        if (Trie) {
            auto key = BuildKey(str);
            const TTrieData* unknownData = &UNKNOWN_DATA;
            if (str.find(BIGRAM_DELIMITER) != TUtf16String::npos) {
                unknownData = &UNKNOWN_BIGRAM_DATA;
            }
            return Getter(Trie->GetDefault(key, *unknownData));
        } else {
            return 0;
        }
    }

    float TLevensteinWordDist::DeleteWeight(const TUtf16String& str) const {
        return InsertWeight(str);
    }
}
