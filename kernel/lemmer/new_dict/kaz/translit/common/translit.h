#pragma once

#include <kernel/lemmer/new_dict/kaz/main/kaz.h>
#include <kernel/lemmer/untranslit/ngrams/ngrams.h>
#include <kernel/lemmer/untranslit/trie/trie.h>

namespace {
    struct TKazAlphaMap: public NLemmer::NDetail::TAlphaMap {
        TKazAlphaMap()
            : TAlphaMap(NLemmer::GetAlphaRulesUnsafe(LANG_KAZ))
        {
        }
    };
}

namespace NLemmer {
    namespace NData {
        extern NLemmer::NDetail::TNGramData NGramDataKk;
    }
}

extern TTrieNode<wchar16> TrieUntranslitKaz[];
extern const wchar16* TrieUntranslitKazData[];

static TAutoPtr<TUntransliter> GetKazUntransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& theWord)
            : TUntransliter(TrieUntranslitKaz, TrieUntranslitKazData, NLemmer::NData::NGramDataKk, *HugeSingleton<TKazAlphaMap>())
        {
            Init(theWord);
        }
    };

    return new T_(word);
}

extern TTrieNode<wchar16> TrieTranslitKaz[];
extern const wchar16* TrieTranslitKazData[];

static TAutoPtr<TUntransliter> GetKazTransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& theWord)
            : TUntransliter(TrieTranslitKaz, TrieTranslitKazData, NLemmer::NDetail::TNGramData::Empty, NLemmer::NDetail::TAlphaMap::Empty())
        {
            Init(theWord);
        }
    };

    return new T_(word);
}

class TKazkishTransliterInitializer {
public:
    TKazkishTransliterInitializer() {
        GetKazUntransliter(TUtf16String());
        GetKazTransliter(TUtf16String());
    }
};

//static TKazkishTransliterInitializer KazkishTransliterInitializer;
