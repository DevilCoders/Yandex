#pragma once

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <kernel/lemmer/core/language.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>

namespace NAutomorphology {
    struct TParadigmWord {
        size_t PrefixLength = 0;
        size_t SuffixLength = 0;
        TUtf16String Text;
        TParadigmWord(const TUtf16String& text, size_t prefixLength, size_t suffixLength)
            : PrefixLength(prefixLength)
            , SuffixLength(suffixLength)
            , Text(text)
        {
        }
    };
    using TParadigm = TVector<TParadigmWord>;
    TParadigm SingleWordParadigm(const TUtf16String& text);
};

class TSimpleCompactLister {
private:
    TVector<TUtf16String> Suffixes;
    TVector<TUtf16String> Prefixes;
    TVector<ui32> SuffixParadigms;

    TCompactTrie<wchar16, ui64> WordParadigms;
    mutable TString DataFingerprint;

public:
    static constexpr char PREFIX_SUFFIX_DELIMITER = '$';
    static constexpr size_t PREFIX_BITS = 16;
    static constexpr size_t STEM_BITS = 16;
    static constexpr size_t PARADIGM_BITS = 32;

    TSimpleCompactLister(IInputStream* wordsIn, IInputStream* suffixesIn, IInputStream* paradigmsIn);

    bool IsWord(const TUtf16String& word) const;

    NAutomorphology::TParadigm GetForms(const TUtf16String& word) const;
    TString Fingerprint() const;
};

class TAutomorphologyLanguage: public TLanguage {
public:
    size_t RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const override;
    TAutoPtr<NLemmer::TFormGenerator> Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter = nullptr) const override;
    TAutoPtr<NLemmer::TGrammarFiltr> GetGrammFiltr(const char* gram) const override;

    TString DictionaryFingerprint() const override {
        return SimpleLister.Fingerprint();
    }

    static TSimpleSharedPtr<TAutomorphologyLanguage> LoadLanguage(ELanguage id, IInputStream* wordsIn, IInputStream* suffixesIn, IInputStream* paradigmsIn);
    static TSimpleSharedPtr<TAutomorphologyLanguage> LoadLanguage(ELanguage id, const unsigned char* data, size_t dataSize);
    static TSimpleSharedPtr<TAutomorphologyLanguage> LoadLanguage(ELanguage id, const TString& path);

    bool HasMeaningfulLemmas() const override {
        return false;
    }

    ~TAutomorphologyLanguage() override {
    }

private:
    TAutomorphologyLanguage(ELanguage id, bool dontRegister, IInputStream* wordsIn, IInputStream* suffixesIn, IInputStream* paradigmsIn);

    static THashMap<ELanguage, TSimpleSharedPtr<TAutomorphologyLanguage>> LoadedLangs;

    TSimpleCompactLister SimpleLister;
};
