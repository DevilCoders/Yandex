#pragma once
#include <kernel/search_types/search_types.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <library/cpp/wordpos/wordpos.h>

struct TWordLanguage {
    TChar LemmaText[MAXWORD_BUF];
    size_t LemmaLen;
    ELanguage Language;
};

struct TLemmatizer {
    virtual void LemmatizeIndexWord(const TWtringBuf& word, ELanguage language, TVector<TWordLanguage>& res, TWLemmaArray& buffer) const = 0;
    virtual bool NeedLemmatizing(TKeyLemmaInfo &keyLemma, const TStringBuf& form, ELanguage lang) const = 0;
    virtual bool NeedLemmatizing(TKeyLemmaInfo &keyLemma, char forms[][MAXKEY_BUF], int formsCount) const = 0;
    virtual ~TLemmatizer(){};
};

int IndexConvert(const char *from, const char *to, const TLemmatizer &lemmatizer, const bool verbose);
