#pragma once

#include <kernel/lemmer/new_dict/tur/main/tur.h>
#include <kernel/lemmer/untranslit/ngrams/ngrams.h>
#include <kernel/lemmer/untranslit/trie/trie.h>

namespace {
    struct TTurAlphaMap: public NLemmer::NDetail::TAlphaMap {
        TTurAlphaMap()
            : TAlphaMap(NLemmer::GetAlphaRulesUnsafe(LANG_TUR))
        {
        }
    };
}

namespace NLemmer {
    namespace NData {
        extern NLemmer::NDetail::TNGramData NGramDataTr;
    }
}

extern TTrieNode<wchar16> TrieUntranslitTur[];
extern const wchar16* TrieUntranslitTurData[];

static TAutoPtr<TUntransliter> GetTurUntransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& theWord)
            : TUntransliter(TrieUntranslitTur, TrieUntranslitTurData, NLemmer::NData::NGramDataTr, *HugeSingleton<TTurAlphaMap>())
        {
            Init(theWord);
        }
    };

    return new T_(word);
}

extern TTrieNode<wchar16> TrieTranslitTur[];
extern const wchar16* TrieTranslitTurData[];

static TAutoPtr<TUntransliter> GetTurTransliter(const TUtf16String& word) {
    struct T_: public TUntransliter {
        T_(const TUtf16String& theWord)
            : TUntransliter(TrieTranslitTur, TrieTranslitTurData, NLemmer::NDetail::TNGramData::Empty, NLemmer::NDetail::TAlphaMap::Empty())
        {
            Init(theWord);
        }
    };

    return new T_(word);
}

class TTurkishTransliterInitializer {
public:
    TTurkishTransliterInitializer() {
        GetTurUntransliter(TUtf16String());
        GetTurTransliter(TUtf16String());
    }
};

//static TTurkishTransliterInitializer TurkishTransliterInitializer;
