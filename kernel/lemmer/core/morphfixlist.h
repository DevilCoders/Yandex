#pragma once

#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmaforms.h>

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>

class TLemmaForms;

class IInputStream;

namespace NLemmer {
    class TMorphFixLanguage: public TLanguage {
    private:
        THolder<TLanguage> Language;

    public:
        TMorphFixLanguage(TLanguage* language)
            : TLanguage(language->Id, language->Version, true)
            , Language(language)
        {
        }

        struct TRealNodes {
            size_t FormsCount;
            TVector<TLemmaForms> LemmaForms;
        };

        THashMap<TUtf16String, TRealNodes> RealNodesHash;

    protected:
        size_t RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const override;
        TAutoPtr<NLemmer::TFormGenerator> Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter) const override;
    };

    class TMorphFixList {
    private:
        THashMap<ELanguage, THolder<TMorphFixLanguage>> Languages;

    public:
        void AddLanguage(TLanguage* language, const TSet<TUtf16String>& forms);

        static const TLanguage* SafeGetLanguage(const TMorphFixList* morphFixList, ELanguage lang);

        const TVector<TLemmaForms>* GetRealNodes(ELanguage lang, const TUtf16String& form, size_t formsCount) const;
        void InitRealNodes();
    };

    void InitRealNodes();
}
