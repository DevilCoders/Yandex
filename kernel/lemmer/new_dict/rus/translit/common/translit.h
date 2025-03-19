#pragma once

#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/untranslit/ngrams/ngrams.h>
#include <kernel/lemmer/untranslit/trie/trie.h>
#include <kernel/lemmer/untranslit/untranslit.h>
#include <util/generic/singleton.h>

namespace NLemmer {
    namespace NData {
        const TTranslationDict* GetTranslationRuslit();
        extern NLemmer::NDetail::TNGramData NGramDataRu;
    }
}

namespace {
    struct TBadBeforeSigns {
        TUtf16String Bad;
        TBadBeforeSigns()
            : Bad(u"ьъёуеыаоэяию")
        {
        }
    };

    bool BadBeforeSigns(wchar16 c) {
        const TUtf16String& bad = Singleton<TBadBeforeSigns>()->Bad;
        return bad.find(c) < bad.length();
    }
    struct TRusAlphaMap: public NLemmer::NDetail::TAlphaMap {
        TRusAlphaMap()
            : TAlphaMap(NLemmer::GetAlphaRulesUnsafe(LANG_RUS))
        {
        }
    };
}

extern TTrieNode<wchar16> TrieUntranslitRus[];
extern const wchar16* TrieUntranslitRusData[];

static TAutoPtr<TUntransliter> GetRusUntransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& aWord)
            : TUntransliter(TrieUntranslitRus, TrieUntranslitRusData, NLemmer::NData::NGramDataRu, *HugeSingleton<TRusAlphaMap>())
        {
            Init(aWord);
        }

        bool Unacceptable(const TUtf16String& str, size_t diff) const override {
            if (str.empty())
                return false;
            Y_ASSERT(diff <= str.length());
            for (size_t k = 1; k <= diff; k++) {
                size_t i = str.length() - k;
                if (str[i] == 0x44C || str[i] == 0x44A) {
                    if (i == 0 || BadBeforeSigns(str[i - 1]))
                        return true;
                }
            }
            return false;
        }
    };

    return new T_(word);
}

extern TTrieNode<wchar16> TrieTranslitRus[];
extern const wchar16* TrieTranslitRusData[];

static TAutoPtr<TUntransliter> GetRusTransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& aWord)
            : TUntransliter(TrieTranslitRus, TrieTranslitRusData, NLemmer::NDetail::TNGramData::Empty, NLemmer::NDetail::TAlphaMap::Empty())
        {
            Init(aWord);
        }
    };

    return new T_(word);
}

class TRussianTransliterInitializer {
public:
    TRussianTransliterInitializer() {
        GetRusUntransliter(TUtf16String());
        GetRusTransliter(TUtf16String());
    }
};
