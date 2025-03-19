#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/core/lemmer.h>
#include <library/cpp/langmask/langmask.h>

namespace NInfl {

// Declared in this file:
class TComplexWord;
class TSimpleAutoColloc;


// forward
struct TYemmaSelector;

class TComplexWord {
public:
    TComplexWord(const TLangMask& langMask, const TUtf16String& wordText, const TUtf16String& lemmaText = TUtf16String(),
                 const TGramBitSet& knownGrammems = TGramBitSet(), const TYandexLemma* knownLemma = nullptr,
                 bool UseKnownGrammemsInLemmer = false);

    // constructor for non-temporary strings (owned by someone else)
    TComplexWord(const TLangMask& langMask, const TWtringBuf& wordText, const TWtringBuf& lemmaText = TWtringBuf(),
                 const TGramBitSet& knownGrammems = TGramBitSet(), const TYandexLemma* knownLemma = nullptr,
                 bool useKnownGrammemsInLemmer = false);

    TComplexWord(const TUtf16String& wordText, const TVector<TYandexLemma>& yemmas);
    TComplexWord(const TWtringBuf& wordText, const TVector<TYandexLemma>& yemmas);

    // leave only lemmas with specified lang
    void FilterLang(ELanguage lang);

    bool Inflect(const TGramBitSet& grammems, TUtf16String& res, TGramBitSet* resgram = nullptr) const;
    bool Normalize(TUtf16String& res, TGramBitSet* resgrammems = nullptr) const;


    // simply copy capitalization masks from @src to @dst, word by word
    static void CopyCapitalization(TWtringBuf src, TUtf16String& dst);

    // more complex behaviour: look at the number of words in @src and @dst and find some common patterns
    static void MimicCapitalization(TWtringBuf src, TUtf16String& dst);

    // case-insensitive word text comparison
    static bool IsEqualText(const TWtringBuf& a, const TWtringBuf& b);


    static inline TGramBitSet ExtractFormGrammems(const TYandexLemma& word, size_t flexIndex) {
        TGramBitSet grammems;
        Y_ASSERT(flexIndex < word.FlexGramNum());
        NSpike::ToGramBitset(word.GetStemGram(), word.GetFlexGram()[flexIndex], grammems);
        return grammems;
    }

    inline TGramBitSet Grammems() const {
        return HasYemma ? SelectedYemmaGrammems() : KnownGrammems;
    }

    const TVector<TYandexLemma>& GetCandidates() const {
        return CandidateYemmas;
    };

    TString DebugString() const;


private:
    void ResetYemma(const TYandexLemma* knownLemma);
    void Analyze();
    bool SelectBestAgreedYemma(const TGramBitSet& agree, TYemmaSelector& res);

    bool InflectInt(const TGramBitSet& grammems, TUtf16String& res, TGramBitSet* resgram) const;
    bool InflectYemma(const TGramBitSet& grammems, TUtf16String& res, TGramBitSet* resgram) const;
    bool SplitAndModify(size_t wordDelim, const TGramBitSet& grammems,
                        TUtf16String& res, TGramBitSet* resgrammems) const;

    // selected yemma grammems
    inline TGramBitSet SelectedYemmaGrammems() const {
        Y_ASSERT(HasYemma);
        return ExtractFormGrammems(Yemma, 0);
    }

    bool AllYemmasHasAnyGrammeme(const TGramBitSet& allowedGrammems, const TGramBitSet& forbiddenGrammemes = Default<TGramBitSet>()) const;

    // Checks that some yemma has all @requiredGrammemes, but none of @forbiddenGrammemes
    bool SomeYemmaHasGrammemes(const TGramBitSet& requiredGrammemes, const TGramBitSet& forbiddenGrammemes = Default<TGramBitSet>()) const;

    bool SomeYemmaHasAnyGrammeme(const TGramBitSet& grammemes) const;


private:
    TLangMask LangMask;
    TUtf16String WordTextBuffer, LemmaTextBuffer;
    TWtringBuf WordText, LemmaText;
    TGramBitSet KnownGrammems;
    bool UseKnownGrammemsInLemmer;

    TVector<TYandexLemma> CandidateYemmas;

    TYandexLemma Yemma;
    bool HasYemma;
    bool Analyzed;      // CandidateYemmas were already found using NLemmer::AnalyzeWord
    bool DoNotInflect;

    friend class TSimpleAutoColloc;
};


// A collocation with one main word and all other words dependent from the main one.
// The type of dependency (dependent features) are figured automatically.
class TSimpleAutoColloc {
public:
    TSimpleAutoColloc()
        : Main(0)
    {
    }

    void AddWord(const TComplexWord& word, bool isMain = false) {
        Words.push_back(word);
        if (isMain)
            Main = Words.size() - 1;

        for (const auto& cand : word.GetCandidates()) {
            if (!cand.IsBastard())
                ++LangStat[cand.GetLanguage()];
        }
    }

    void AddMainWord(const TComplexWord& word) {
        AddWord(word, true);
    }

    // Find first noun in the sequence and set it as main
    void GuessMainWord();

    // Guess most probable language and remove all others from words
    void GuessOneLang();

    // Re-analyze dependent words to select lemmas best agreed with the main word
    void ReAgree();

    bool Inflect(const TGramBitSet& grammems, TVector<TUtf16String>& res, TGramBitSet* resgram = nullptr, bool pluralization = false) const;
    // TODO: bool Modify(const TGramBitSet& grammems, TUtf16String& res, TGramBitSet* resgram = NULL) const;
    bool Normalize(TVector<TUtf16String>& res) const;

    TString DebugString() const;

private:
    void AgreeWithMain(const TGramBitSet& gram, TYemmaSelector* best = nullptr);
    bool SpecialGeoInflectionNeeded() const;
    bool DetectGovernment();

private:
    TVector<TComplexWord> Words;
    size_t Main;
    THashMap<ELanguage, int> LangStat;
};



}   // NInfl
