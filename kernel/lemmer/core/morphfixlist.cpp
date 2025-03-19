#include "morphfixlist.h"
#include "wordinstance.h"

#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/language.h>

#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/null.h>
#include <util/string/strip.h>
#include <util/system/spinlock.h>

namespace NLemmer {
    size_t TMorphFixLanguage::RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const {
        Y_ASSERT(opt.UseFixList);
        size_t base = out.size();
        size_t r = Language->RecognizeInternal(word, length, out, opt);
        for (size_t i = 0; i < r; ++i) {
            NLemmerAux::TYandexLemmaSetter(out[base + i]).SetQuality(TYandexLemma::QFix);
        }
        return r;
    }

    TAutoPtr<NLemmer::TFormGenerator> TMorphFixLanguage::Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter) const {
        return Language->Generator(lemma, filter);
    }

    const TLanguage* TMorphFixList::SafeGetLanguage(const TMorphFixList* morphFixList, ELanguage lang) {
        if (!morphFixList) {
            return nullptr;
        }
        const auto& it = morphFixList->Languages.find(lang);
        if (it == morphFixList->Languages.end()) {
            return nullptr;
        }
        return it->second.Get();
    }

    const TVector<TLemmaForms>* TMorphFixList::GetRealNodes(ELanguage lang, const TUtf16String& form, size_t formsCount) const {
        auto langIter = Languages.find(lang);
        if (langIter == Languages.end())
            return nullptr;
        const auto& bh = langIter->second;
        auto formIter = bh->RealNodesHash.find(form);
        if (formIter == bh->RealNodesHash.end())
            return nullptr;
        if (formIter->second.FormsCount != formsCount)
            return nullptr;
        return &formIter->second.LemmaForms;
    }

    void TMorphFixList::InitRealNodes() {
        for (auto& langIter : Languages) {
            ELanguage lang = langIter.first;
            for (auto& formIter : langIter.second->RealNodesHash) {
                const TUtf16String& form = formIter.first;
                size_t formsCount = 0;
                formIter.second.LemmaForms = TWordInstanceUpdate::GenerateRealNodes(lang, form, &formsCount);
                formIter.second.FormsCount = formsCount;
            }
        }
    }

    void TMorphFixList::AddLanguage(TLanguage* language, const TSet<TUtf16String>& forms) {
        //Languages[language->Id] = TMorphFixLanguage(language);
        auto lang = MakeHolder<TMorphFixLanguage>(language);
        for (const auto& form : forms) {
            lang->RealNodesHash[form];
        }
        Languages.emplace(language->Id, std::move(lang));
    }
}

namespace {
    struct TMorphFixListHolder {
        THolder<NLemmer::TMorphFixList> FixList;

        void InitFixList(THolder<NLemmer::TMorphFixList>& fixList, bool replaceAlreadyLoaded) {
            static TAdaptiveLock lock;
            TGuard guard(lock);

            if (!replaceAlreadyLoaded && (FixList.Get() != nullptr))
                return;

            FixList.Swap(fixList);
        }
    };
}

namespace NLemmer {
    void SetMorphFixList(THolder<NLemmer::TMorphFixList>& fixList, bool replaceAlreadyLoaded) {
        Singleton<TMorphFixListHolder>()->InitFixList(fixList, replaceAlreadyLoaded);
    }

    const TMorphFixList* GetMorphFixList() {
        return Singleton<TMorphFixListHolder>()->FixList.Get();
    }

    void InitRealNodes() {
        Singleton<TMorphFixListHolder>()->FixList.Get()->InitRealNodes();
    }
}
