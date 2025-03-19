#pragma once

#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/untranslit/ngrams/ngrams.h>
#include <kernel/lemmer/untranslit/trie/trie.h>
#include <kernel/lemmer/untranslit/untranslit.h>
#include <util/generic/singleton.h>

namespace NLemmer {
    namespace NData {
        const TTranslationDict* GetTranslationUkrlit();
        extern NLemmer::NDetail::TNGramData NGramDataUk;
    }
}

namespace {
    static bool BadBeforeSigns(wchar16 c) {
        static const wchar16 badBeforeSigns[] = {0x44C, 0x027,
                                               0x430, 0x435, 0x454, 0x438, 0x456, 0x43E, 0x443, 0x44E, 0x44F, 0x451, 0};
        return TWtringBuf{badBeforeSigns}.Contains(c);
    }

    struct TUkrAlphaMap: public NLemmer::NDetail::TAlphaMap {
        TUkrAlphaMap()
            : TAlphaMap(NLemmer::GetAlphaRules(LANG_UKR))
        {
        }
    };
}

extern TTrieNode<wchar16> TrieUntranslitUkr[];
extern const wchar16* TrieUntranslitUkrData[];

inline TAutoPtr<TUntransliter> GetUkrUntransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& theWord)
            : TUntransliter(TrieUntranslitUkr, TrieUntranslitUkrData, NLemmer::NData::NGramDataUk, *HugeSingleton<TUkrAlphaMap>())
        {
            Init(theWord);
        }

        bool Unacceptable(const TUtf16String& str, size_t diff) const override {
            if (str.empty())
                return false;
            Y_ASSERT(diff <= str.length());
            for (size_t k = 1; k <= diff; k++) {
                size_t i = str.length() - k;
                if (str[i] == 0x44C || str[i] == 0x027) {
                    if (i == 0 || BadBeforeSigns(str[i - 1]))
                        return true;
                }
            }
            return false;
        }
    };

    return new T_(word);
}

extern TTrieNode<wchar16> TrieTranslitUkr[];
extern const wchar16* TrieTranslitUkrData[];

inline TAutoPtr<TUntransliter> GetUkrTransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& theWord)
            : TUntransliter(TrieTranslitUkr, TrieTranslitUkrData, NLemmer::NDetail::TNGramData::Empty, NLemmer::NDetail::TAlphaMap::Empty())
        {
            Init(theWord);
        }
    };

    return new T_(word);
}
