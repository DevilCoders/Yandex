#pragma once

// TLanguageContext contains bits & pieces needed to lemmatize & classify the word properly,
// such as the list of stopwords, the current language mask and priority list.
#include <library/cpp/stopwords/stopwords.h>
#include "decimator.h"

#include <library/cpp/langmask/langmask.h>

class TLemmerCache;

class TLanguageContext {
private:
    TLangMask LangMask;
    TLangMask DisabledLanguages;
    TLangMask TranslitLanguages;
    const ELanguage* LangOrder;
    const TWordFilter* StopWords;
    const TFormDecimator* Decimator;

public:
    bool AllowDeprecated;
    bool FilterForms;
    const TLemmerCache* LemmerCache;

    TLanguageContext(const TLanguageContext& other) = default;

    explicit TLanguageContext(const TLangMask& mask = LI_BASIC_LANGUAGES, const ELanguage* langlist = nullptr,
                              const TWordFilter& sw = TWordFilter::EmptyFilter,
                              const TFormDecimator& dcm = TFormDecimator::EmptyDecimator);

    bool IsStopWord(const char* word) const;
    bool IsStopWord(const wchar16* word) const;

    TLangMask& GetMutableLangMask();
    const TLangMask& GetLangMask() const;
    void SetLangMask(const TLangMask& id);

    TLangMask& GetMutableDisabledLanguages();
    const TLangMask& GetDisabledLanguages() const;
    void SetDisabledLanguages(const TLangMask& id);

    TLangMask& GetMutableTranslitLanguages();
    const TLangMask& GetTranslitLanguages() const;
    void SetTranslitLanguages(const TLangMask& id);

    const ELanguage* GetLangOrder() const;
    void SetLangOrder(const ELanguage* id);

    const TWordFilter& GetStopWords() const;
    void SetStopWords(const TWordFilter& wf);

    const TFormDecimator& GetDecimator() const;
    void SetDecimator(const TFormDecimator& dcm);
};
