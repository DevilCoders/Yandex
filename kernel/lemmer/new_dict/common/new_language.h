#pragma once

#include <util/generic/string.h>
#include <util/generic/ptr.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/new_engine/new_lemmer.h>

class TNewLanguage: public TLanguage {
public:
    size_t RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const override;
    TAutoPtr<NLemmer::TFormGenerator> Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter = nullptr) const override;
    TAutoPtr<NLemmer::TGrammarFiltr> GetGrammFiltr(const char* gram) const override;
    TString DictionaryFingerprint() const override {
        return GetLemmer() ? GetLemmer()->Fingerprint() : Default<TString>();
    }

    bool Spellcheck(const wchar16* word, size_t len) const override;

protected:
    TNewLanguage(ELanguage lang, bool dontRegister = false, int version = 1)
        : TLanguage(lang, dontRegister, version)
        , Lemmer(nullptr)
    {
    }

    const TNewLemmer* GetLemmer() const {
        return Lemmer.Get();
    }

    THolder<TNewLemmer> Lemmer;

public:
    void Init(const TBlob& blob) {
        Lemmer.Reset(new TNewLemmer(blob.AsCharPtr(), blob.Size(), NLemmer::GetAlphaRules(Id)));
    }
};

template <ELanguage LANG>
class TLanguageTemplate: public TNewLanguage {
protected:
    TLanguageTemplate()
        : TNewLanguage(LANG)
    {
    }
};
