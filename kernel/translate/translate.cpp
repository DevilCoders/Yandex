#include <kernel/translate/common/translate.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/translate/translatedict.h>

#include "translate.h"

namespace {
    const TTranslationDict* GetTranslation(ELanguage language) {
        switch (language) {
            case LANG_RUS:
                return NTranslate::NData::GetTranslationRus();
            case LANG_TUR:
                return NTranslate::NData::GetTranslationTur();
            case LANG_UKR:
                return NTranslate::NData::GetTranslationUkr();
            default:
                return nullptr;
        }
    }

    struct TTranslator {
    protected:
        const TLanguage* LangEng;
        const TLanguage* Current;
        const TTranslationDict* Translator;
        const TYandexLemma::TQuality Qual;
    protected:
        TTranslator(const TLanguage* curr, const TTranslationDict* translator, TYandexLemma::TQuality qual)
            : LangEng(NLemmer::GetLanguageById(LANG_ENG))
            , Current(curr)
            , Translator(translator)
            , Qual(qual)
        {
        }
        TTranslator(ELanguage language, TYandexLemma::TQuality qual)
            : TTranslator(NLemmer::GetLanguageById(language), GetTranslation(language), qual)
        {
        }
        virtual const TLanguage* From() const = 0;
        virtual const TLanguage* To() const = 0;
        virtual size_t TranslateInt(const TTranslationDict::TArticle& art, TVector<TTranslationDict::TArticle>& result) const = 0;
        virtual ~TTranslator() {} // make gcc happy
        size_t Translate(const wchar16* word, size_t length, TWLemmaArray& out, size_t max, const char*) {
            if (!Translator) {
                return 0;
            }
            if (!Current) {
                return 0;
            }
            size_t initialSize = out.size();
            {
                size_t num = TranslateInternal(word, length, out, max);
                for (size_t i = 0; i < num; ++i) {
                    NLemmerAux::TYandexLemmaSetter set(out[initialSize + i]);
                    set.SetLanguage(To()->Id);
                    set.SetInitialForm(word, length);
                    set.SetNormalizedForm(word, length);
                    set.SetQuality(Qual);
                }
                if (num)
                    return num;
            }

            TWLemmaArray engLemmas;
            From()->Recognize(word, length, engLemmas);
            TWUniqueLemmaArray uniqs;
            FillUniqLemmas(uniqs, engLemmas);

            size_t count = 0;
            for (size_t i = 0; i < uniqs.size(); i++)
                count += TranslateInternal(uniqs[i]->GetText(), uniqs[i]->GetTextLength(), out, max);

            for (size_t i = 0; i < count; ++i) {
                NLemmerAux::TYandexLemmaSetter set(out[i + initialSize]);
                set.SetLanguage(To()->Id);
                set.SetInitialForm(uniqs[0]->GetInitialForm(), uniqs[0]->GetInitialFormLength());
                set.SetNormalizedForm(uniqs[0]->GetNormalizedForm(), uniqs[0]->GetNormalizedFormLength());
                set.SetQuality(Qual);
            }
            return count;
        }
    private:
        size_t TranslateInternal(const wchar16* word, size_t length, TWLemmaArray& out, size_t maxw, const char* needGramm = "") {
            static const char gr[2] = {NTGrammarProcessing::tg2ch(gComposite), 0};
            TVector<TTranslationDict::TArticle> res;
            wchar16* buf = (wchar16*) alloca((length + 1) * sizeof(wchar16));
            memcpy(buf, word, length * sizeof(wchar16));
            buf[length] = 0;
            TranslateInt(TTranslationDict::TArticle(buf, needGramm ? TGramBitSet::FromBytes(needGramm) : TGramBitSet()), res);
            size_t i = 0;
            for (; i < res.size() && i < maxw; ++i) {
                out.push_back(TYandexLemma());
                NLemmerAux::TYandexLemmaSetter set(out.back());
                set.SetLemma(res[i].Word, std::char_traits<wchar16>::length(res[i].Word), 0, 0, 0, gr);
                set.AddFlexGr("");
            }

            return i;
        }
    };

    struct TFromEnglish: private TTranslator {
    public:
        TFromEnglish(ELanguage language)
            : TTranslator (language, TYandexLemma::QFromEnglish)
        {
        }
        using TTranslator::Translate;
    private:
        const TLanguage* From() const override {
            return LangEng;
        }
        const TLanguage* To() const override {
            return Current;
        }
        size_t TranslateInt(const TTranslationDict::TArticle& art, TVector<TTranslationDict::TArticle>& result) const override {
            return Translator->FromEnglish(art, result);
        }
    };

    struct TToEnglish: private TTranslator {
    public:
        TToEnglish(ELanguage language)
            : TTranslator (language, TYandexLemma::QToEnglish)
        {
        }
        using TTranslator::Translate;
    private:
        const TLanguage* From() const override {
            return Current;
        }
        const TLanguage* To() const override {
            return LangEng;
        }
        size_t TranslateInt(const TTranslationDict::TArticle& art, TVector<TTranslationDict::TArticle>& result) const override {
            return Translator->ToEnglish(art, result);
        }
    };
}

namespace NTranslate {
    size_t ToEnglish(const TWtringBuf word, ELanguage language, TWLemmaArray& out, size_t max, const char* needGramm) {
        TToEnglish translator(language);
        return translator.Translate(word.data(), word.size(), out, max, needGramm);
    }
    size_t FromEnglish(const TWtringBuf word, ELanguage language, TWLemmaArray& out, size_t max, const char* needGramm) {
        TFromEnglish translator(language);
        return translator.Translate(word.data(), word.size(), out, max, needGramm);
    }
}
