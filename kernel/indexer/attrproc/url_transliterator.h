#pragma once

#include <kernel/lemmer/core/language.h>
#include <library/cpp/langmask/langmask.h>

#include <ysite/yandex/common/urltok.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

class TUrlTransliteratorItem {
public:
    friend class TSerializer<TUrlTransliteratorItem>;
    enum ELemmaType {
        LtOriginal = 0,
        LtTranslit,
        LtTranslate
    };
private:
    TUtf16String Part;
    TUtf16String Form;
    TUtf16String Lemma;
    ELanguage Language;
    size_t Index;
    bool InCGI;
    bool Titled;
    ELemmaType LemmaType;
private:
    void ConvertPartToLower();

public:
    TUrlTransliteratorItem()
        : Language(LANG_UNK)
        , InCGI(0)
        , Titled(0)
        , LemmaType(LtOriginal)
    {
    }

    const TUtf16String& GetLowerCasePart() const;
    bool IsPartTitled() const;
    void SetPart(const TUtf16String& part);
    const TUtf16String& GetLowerCaseLemma() const;
    void SetForma(const TUtf16String& form);
    const TUtf16String& GetForma() const;
    void SetLemma(const TUtf16String& lemma);
    ELanguage GetLanguage() const;
    void SetLanguage(ELanguage lang);
    size_t GetIndex() const;
    void SetIndex(size_t index);
    bool GetInCGI() const;
    void SetInCGI(bool value);
    ELemmaType GetLemmaType() const;
    void SetLemmaType(ELemmaType value);
};

template<>
class TSerializer<TUrlTransliteratorItem> {
public:
    static void Load(IInputStream* in, TUrlTransliteratorItem& item);
    static void Save(IOutputStream* out, const TUrlTransliteratorItem& item);
};

class TUrlTransliterator {
public:
    struct TLemma {
        TUtf16String Lemma;
        TUtf16String Forma;
        ELanguage Language;
        TUrlTransliteratorItem::ELemmaType Type;
    };
    typedef TVector<TLemma> TLemmas;

    class TTransliteratorCache {
    public:
        struct TUrlRequest;
    private:
        class TImpl;
        THolder<TImpl> Impl;
    public:
        TTransliteratorCache();
        ~TTransliteratorCache();
        const TUrlTransliterator::TLemmas* Transliterate(const TUrlRequest& req, const TBlob* tokSplitData);
    };

    TUrlTransliterator(TTransliteratorCache& transliteratorCache, const TString& url, ELanguage docLang, const TBlob* ptrTokSplitData = nullptr);
    bool Has();
    void Next(TUrlTransliteratorItem& item);

private:
    const size_t MAX_FIRST_MIXEDCASE_TO_TRANSLIT = 2;

private:
    TTransliteratorCache& TransliteratorCache;
    bool HasNextToken;
    TLemma NextToken;
    TURLTokenizer Tokenizer;
    size_t URLPartIndex;
    size_t MixedCaseTokenCount;
    TString URLPart;
    ELanguage PriorityLang;
    TLangMask OtherLangs;
    const TBlob* TokSplitData;
    TLemmas Number;

    const TLemmas* Lemmas;
    size_t LemmaIndex;
    bool Advance();
};
