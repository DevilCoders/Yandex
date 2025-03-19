#include "langcontext.h"

TLanguageContext::TLanguageContext(const TLangMask& mask, const ELanguage* langlist,
                                   const TWordFilter& sw,
                                   const TFormDecimator& dcm)
    : LangMask(mask)
    , LangOrder(langlist)
    , StopWords(&sw)
    , Decimator(&dcm)
    , AllowDeprecated(true)
    , FilterForms(true)
    , LemmerCache(nullptr)
{
}

bool TLanguageContext::IsStopWord(const char* word) const {
    return StopWords->IsStopWord(word);
}

bool TLanguageContext::IsStopWord(const wchar16* word) const {
    return StopWords->IsStopWord(word);
}

TLangMask& TLanguageContext::GetMutableLangMask() {
    return LangMask;
}

const TLangMask& TLanguageContext::GetLangMask() const {
    return LangMask;
}

void TLanguageContext::SetLangMask(const TLangMask& id) {
    LangMask = id;
}

TLangMask& TLanguageContext::GetMutableDisabledLanguages() {
    return DisabledLanguages;
}

const TLangMask& TLanguageContext::GetDisabledLanguages() const {
    return DisabledLanguages;
}

void TLanguageContext::SetDisabledLanguages(const TLangMask& id) {
    DisabledLanguages = id;
}

TLangMask& TLanguageContext::GetMutableTranslitLanguages() {
    return TranslitLanguages;
}

const TLangMask& TLanguageContext::GetTranslitLanguages() const {
    return TranslitLanguages;
}

void TLanguageContext::SetTranslitLanguages(const TLangMask& id) {
    TranslitLanguages = id;
}

const ELanguage* TLanguageContext::GetLangOrder() const {
    return LangOrder;
}
void TLanguageContext::SetLangOrder(const ELanguage* id) {
    LangOrder = id;
}

const TWordFilter& TLanguageContext::GetStopWords() const {
    return *StopWords;
}

void TLanguageContext::SetStopWords(const TWordFilter& wf) {
    StopWords = &wf;
}

const TFormDecimator& TLanguageContext::GetDecimator() const {
    return *Decimator;
}

void TLanguageContext::SetDecimator(const TFormDecimator& dcm) {
    Decimator = &dcm;
}
