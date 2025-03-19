#include "from_text.h"

#include <kernel/lemmer/core/morphfixlist.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/new_dict/common/new_language.h>
#include <kernel/lemmer/new_engine/build_lib/common.h>
#include <kernel/lemmer/new_engine/new_generator.h>

#include <util/generic/vector.h>
#include <ctime>

namespace NLemmer {
    class TSelfContainedNewLanguage: public TNewLanguage {
    private:
        TBlob Data;

    public:
        TSelfContainedNewLanguage(const TBlob& blob, ELanguage language)
            : TNewLanguage(language)
            , Data(blob)
        {
            Init(blob);
        }

        size_t RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const override {
            Y_ASSERT(opt.UseFixList);
            size_t base = out.size();
            NLemmer::TRecognizeOpt realOpt(opt);
            realOpt.UseFixList = false;
            size_t r = TNewLanguage::RecognizeInternal(word, length, out, realOpt);
            for (size_t i = 0; i < r; ++i) {
                NLemmerAux::TYandexLemmaSetter(out[base + i]).SetQuality(TYandexLemma::QFix);
            }
            return r;
        }

        TAutoPtr<NLemmer::TFormGenerator> Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter) const override {
            Y_ASSERT(lemma.GetLanguage() == Id);
            return new TNewFormGenerator(lemma, GetLemmer(), filter);
        }
    };

    using TLoadedLanguageWithForms = std::pair<THolder<TLanguage>, TSet<TUtf16String>>;
    TLoadedLanguageWithForms LoadLanguage(IInputStream& src, ELanguage lang) {
        Y_ENSURE(lang != LANG_UNK, "fixlist for unknown language makes no sense");
        NDictBuild::TDictBuildParams params;
        params.BastardFreq = 0;
        TBufferStream curDict;
        params.Language = lang;
        //params.Verbose = true;
        TSet<TUtf16String> forms;
        NDictBuild::MakeAll(src, curDict, params, &forms);
        TBlob dictBlob = TBlob::FromStream(curDict);
        return TLoadedLanguageWithForms(new TSelfContainedNewLanguage(dictBlob, lang), forms);
    }

    TVector<TLoadedLanguageWithForms> ReadLanguagesFromText(IInputStream& src) {
        TVector<TLoadedLanguageWithForms> result;

        THolder<TBufferStream> curData = MakeHolder<TBufferStream>();
        TString line;
        ELanguage lang = LANG_RUS;
        static const char LangPrefix[] = "@language:";
        size_t activeLines = 0;

        while (src.ReadLine(line)) {
            if (line.empty()) {
                continue;
            }
            if (line.StartsWith(LangPrefix[0])) {
                line.to_lower();
                if (!line.StartsWith(LangPrefix)) {
                    continue;
                }
                if (activeLines) {
                    result.emplace_back(LoadLanguage(*curData, lang));
                }
                curData = MakeHolder<TBufferStream>();
                activeLines = 0;
                lang = LanguageByNameStrict(line.substr(sizeof(LangPrefix) / sizeof(LangPrefix[0])));
            } else {
                if (!line.StartsWith('#')) {
                    *curData << line << '\n';
                    ++activeLines;
                }
            }
        }
        if (activeLines) {
            *curData << Endl; //allow no new line in end
            result.emplace_back(LoadLanguage(*curData, lang));
        }
        return result;
    }

    void ReadFixList(IInputStream& src, TMorphFixList* fixList) {
        for (auto& lang : ReadLanguagesFromText(src)) {
            fixList->AddLanguage(lang.first.Release(), lang.second);
        }
    }

    void SetMorphFixList(const char* filename, bool replaceAlreadyLoaded /*= true*/) {
        if (!replaceAlreadyLoaded && GetMorphFixList()) {
            return;
        }
        TBuffered<TUnbufferedFileInput> src(4096, filename);
        THolder<NLemmer::TMorphFixList> fixList(new NLemmer::TMorphFixList);
        ReadFixList(src, fixList.Get());
        SetMorphFixList(fixList, true);
    }

    void SetMorphFixList(IInputStream& src, bool replaceAlreadyLoaded /*= true*/) {
        if (!replaceAlreadyLoaded && GetMorphFixList()) {
            return;
        }
        THolder<NLemmer::TMorphFixList> fixList(new NLemmer::TMorphFixList);
        ReadFixList(src, fixList.Get());
        SetMorphFixList(fixList, replaceAlreadyLoaded);
    }
}
