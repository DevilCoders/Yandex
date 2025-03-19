#pragma once

#include <util/system/defaults.h>
#include <kernel/search_types/search_types.h>
#include <util/system/compat.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/string/vector.h>

#include <library/cpp/token/token_structure.h>
#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/dictlib/grammar_index.h>
#include <kernel/lemmer/untranslit/detranslit.h>
#include <kernel/lemmer/untranslit/untranslit.h>
#include <kernel/lemmer/alpha/abc.h>

#include "formgenerator.h"
#include "lemmer.h"
#include "options.h"
#include "token.h"
#include <util/generic/noncopyable.h>

namespace NLemmer {
    namespace NDetail {
        class TWordAnalyzer;
        class TMultitokenGluer;
    }
}

class TLanguage;
class TTranslationDict;
class TNewLanguage;

namespace NLemmer {
    class TMorphFixLanguage;
};

namespace NLemmer {
    size_t AnalyzeWord(const TWideToken& tokens, TWLemmaArray& out,
                       TLangMask langmask, const ELanguage* doclangs = nullptr, const TAnalyzeWordOpt& opt = TAnalyzeWordOpt());

    inline size_t AnalyzeWord(const wchar16* word, size_t len, TWLemmaArray& out,
                              TLangMask langmask, const ELanguage* doclangs = nullptr, const TAnalyzeWordOpt& opt = TAnalyzeWordOpt()) {
        return AnalyzeWord(TWideToken(word, len), out, langmask, doclangs, opt);
    }

    // Return nullptr for unsupported languages (languages without morphology rules)
    const TLanguage* GetLanguageById(ELanguage id);
    const TLanguage* GetLanguageWithMeaningfulLemmasById(ELanguage id);
    const TLanguage* GetLanguageByVersion(ELanguage id, int version);
    const TLanguage* GetPreviousVersionOfLanguage(ELanguage id, int version);
    const TLanguage* GetLanguageByName(const char* name);
    const TLanguage* GetLanguageByName(const wchar16* name);
    inline const TLanguage* GetLanguageByIdAnyway(ELanguage id) {
        const TLanguage* ret = GetLanguageById(id);
        if (!ret)
            ret = GetLanguageById(LANG_UNK);
        return ret;
    }
    // Classify word casing pattern and composition
    TCharCategory ClassifyCase(const wchar16* word, size_t len);
    // Limits potential languages by looking into the alphabet
    TLangMask ClassifyLanguage(const wchar16* word, size_t len, bool ignoreDigit = false, bool primary = true);
    // Checks if a Unicode character makes part of the language alphabet
    TLangMask GetCharInfo(wchar16 ch, bool primary = true); // obsolete

    //multithreading hack
    void PreInitLanguages(TLangMask langMask = ~TLangMask());
    TLangMask GetLanguagesMask(const TString& str, const TString& delimeters = ", ");

    // Just an array of languages, for ease of enumerating
    typedef TVector<const TLanguage*> TLanguageVector;
    const TLanguageVector& GetLanguageList();

    size_t Generate(const TYandexLemma& lemma, TWordformArray& out, const char* needed_grammar = "");

    TVector<TVector<TUtf16String>> GetWordForms(const TUtf16String& word, ELanguage langId);

    class TMorphFixList;
    const TMorphFixList* GetMorphFixList();
    void SetMorphFixList(THolder<NLemmer::TMorphFixList>& fixList, bool replaceAlreadyLoaded = true);
}
// The core of everything - language descriptor
class TLanguage : TNonCopyable {
public:
    struct TConvertResults {
        static constexpr size_t PreConvertBufSize = TYandexLemma::InitialBufSize;
        static constexpr size_t ConvertBufSize = MAXWORD_BUF;
        TWtringBuf Word;
        wchar16 PreConvertBuf[PreConvertBufSize];
        wchar16 ConvertBuf[ConvertBufSize];
        const wchar16* PreConverted;
        const wchar16* Converted;
        NLemmer::TConvertRet PreConvertRet;
        NLemmer::TConvertRet ConvertRet;

        TConvertResults(const wchar16* const word, size_t length)
            : Word(word, length)
            , PreConverted(word)
            , Converted(word)
            , PreConvertRet(length)
            , ConvertRet(length)
        {
        }
    };

public:
    const ELanguage Id; // numeric identifier
    const int Version;

public:
    size_t Recognize(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt = NLemmer::TRecognizeOpt()) const;
    size_t Generate(const TYandexLemma& lemma, TWordformArray& out, const char* needed_grammar = "") const;

    size_t FromTranslit(const wchar16* word, size_t length, TWLemmaArray& out, size_t max, const NLemmer::TTranslitOpt* opt = &NLemmer::TTranslitOpt::Default()) const;
    // TODO: Add ToTranslit

    virtual bool Spellcheck(const wchar16* word, size_t length) const;

    NLemmer::TConvertRet Normalize(const wchar16* word, size_t length, wchar16* converted, size_t bufLen, NLemmer::TConvertMode mode = ~NLemmer::TConvertMode()) const {
        return AlphaRules().Normalize(word, length, converted, bufLen, mode);
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaRules() const {
        return *NLemmer::GetAlphaRules(Id);
    }
    const char* Name() const { // full name in English
        return FullNameByLanguage(Id);
    }
    const char* Code() const { // ISO code, see RFC 3066
        return IsoNameByLanguage(Id);
    }

    virtual TAutoPtr<NLemmer::TGrammarFiltr> GetGrammFiltr(const char* gram) const;
    virtual bool BelongsToLanguage(const TYandexLemma& lemma) const;

    virtual TAutoPtr<TUntransliter> GetUntransliter(const TUtf16String& word = TUtf16String()) const;
    virtual TAutoPtr<TUntransliter> GetTransliter(const TUtf16String& word = TUtf16String()) const;
    virtual const TTranslationDict* GetUrlTransliteratorDict() const;
    virtual TAutoPtr<IDetransliterator> GetDetransliterator() const;

    virtual ~TLanguage() = 0;

    virtual TString DictionaryFingerprint() const {
        return TString();
    }

    int GetVersion() const {
        return Version;
    }

    virtual bool HasMeaningfulLemmas() const {
        return true;
    }

protected:
    virtual NLemmer::TLllLevel LooksLikeLemma(const TYandexLemma& lemma) const;
    virtual TAutoPtr<NLemmer::TGrammarFiltr> LooksLikeLemmaExt(const TYandexLemma& lemma) const;
    virtual TAutoPtr<NLemmer::TFormGenerator> Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter = nullptr) const;

    virtual size_t RecognizeAdd(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const;
    virtual size_t FromTranslitAdd(const wchar16* word, size_t length, TWLemmaArray& out, size_t max, const NLemmer::TTranslitOpt* opt, bool tryPrecise = false) const;
    virtual bool CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2, bool isTranslit) const;

    TLanguage(ELanguage id, bool dontRegister = false, int version = 0);
    std::pair<size_t, TUtf16String> UntranslitIt(const TConvertResults& cr, size_t length, TWLemmaArray& out, size_t maxw, const NLemmer::TTranslitOpt* opt, bool tryPrecise) const;
    virtual size_t PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults&, const NLemmer::TRecognizeOpt&) const;
    virtual size_t RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const;
    size_t RecognizeInternalFoundling(const wchar16* word, size_t length, TWLemmaArray& out, size_t maxw) const;

    virtual bool CheckOlderVersionsInRecognize(const wchar16* word, size_t length, const TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const;

public:
    virtual void TuneOptions(const NLemmer::TAnalyzeWordOpt& /*awOpt*/, NLemmer::TRecognizeOpt& /*rOpt*/) const {
    }
    void RegisterLanguage(bool forced = false) const;

private:
    friend class TYandexLemma;
    friend class TYandexWordform;
    friend class NLemmer::NDetail::TWordAnalyzer;
    friend class NLemmer::NDetail::TMultitokenGluer;
    friend class TNewLanguage;
    friend class NLemmer::TMorphFixLanguage;
};
