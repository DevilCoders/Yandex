#pragma once

#include "lemmer.h"
#include "language.h"
#include <kernel/search_types/search_types.h>
#include <kernel/lemmer/alpha/diacritics.h>
#include <library/cpp/unicode/normalization/normalization.h>

namespace NLemmerAux {
    class TYandexLemmaSetter {
    private:
        const TYandexLemmaSetter& operator=(const TYandexLemmaSetter&);

    public:
        TYandexLemmaSetter(TYandexLemma& lemma)
            : Lemma(lemma)
        {
        }

        void SetLemmaText(const wchar16* lemmaText, size_t lemmaLen) {
            Lemma.LemmaLen = Min(lemmaLen, (size_t)MAXWORD_BUF - 1);
            memcpy(Lemma.LemmaText, lemmaText, Lemma.LemmaLen * sizeof(wchar16));
            Lemma.LemmaText[Lemma.LemmaLen] = 0;
        }

        void AppendLemmaText(const wchar16* appendText, size_t appendLen) {
            size_t newLen = Min(Lemma.LemmaLen + appendLen, (size_t)MAXWORD_BUF - 1);
            memcpy(Lemma.LemmaText + Lemma.LemmaLen, appendText, appendLen * sizeof(wchar16));
            Lemma.LemmaLen = newLen;
            Lemma.LemmaText[Lemma.LemmaLen] = 0;
        }

        void SetLemmaInfo(ui32 ruleId, size_t prefLen, size_t flexLen, const char* stemGram, size_t paradigmId = 0, size_t lemmaPrefLen = 0);
        void SetLemma(const wchar16* lemmaText, size_t lemmaLen, ui32 ruleId = 0, size_t prefLen = 0, size_t flexLen = 0, const char* stemGram = "");
        void SetWeight(double weight) {
            Lemma.Weight = weight;
        }
        void SetAdditGram(const char* additGram) {
            SetAdditGram((const EGrammar*)additGram);
        }
        void SetAdditGram(const EGrammar* additGram) {
            size_t i = 0;
            for (; additGram[i] && i < TYandexLemma::AddGrammarSize - 1; ++i)
                Lemma.AdditionGram[i] = static_cast<char>(additGram[i]);
            Lemma.AdditionGram[i] = 0;
        }
        size_t AddFlexGr(const char* flexGr, size_t distortions = 0) {
            if (Lemma.GramNum < MAX_GRAM_PER_WORD) {
                Lemma.FlexGram[Lemma.GramNum] = flexGr;
                Lemma.Distortions[Lemma.GramNum] = distortions;
                if (distortions < Lemma.MinDistortion)
                    Lemma.MinDistortion = distortions;
                ++Lemma.GramNum;
            }
            return Lemma.GramNum;
        }
        void SetFlexGrs(const char* const* grs, size_t count) {
            Lemma.GramNum = Min(count, MAX_GRAM_PER_WORD);
            memcpy(Lemma.FlexGram, grs, Lemma.GramNum * sizeof(char*));
        }
        void SetQuality(ui32 quality) {
            Lemma.Quality = quality;
        }
        void SetDepth(ui32 depth) {
            Lemma.Depth = depth;
        }
        void SetLanguage(ELanguage language) {
            Lemma.Language = language;
        }
        void SetCaseFlags(TCharCategory flags) {
            Lemma.Flags = flags;
        }
        void SetToken(size_t tokenPos, size_t tokenSpan) {
            Lemma.TokenPos = tokenPos;
            Lemma.TokenSpan = tokenSpan;
        }
        void SetInitialForm(const wchar16* form, size_t len);
        void SetNormalizedForm(const wchar16* form, size_t len);
        void SetConvertedFormLength(size_t len) {
            Lemma.FormConvertedLen = len;
        }
        void SetForms(const wchar16* form, size_t len);
        void SetLanguageVersion(int version) {
            Lemma.LanguageVersion = version;
        }
        bool AddSuffix(const wchar16* suffix, size_t len);
        void Clear();
        void ResetLemmaToForm(bool convert = true);

    private:
        TYandexLemma& Lemma;
    };

    class TYandexLemmaGetter {
    private:
        const TYandexLemmaGetter& operator=(const TYandexLemmaGetter&);

    public:
        TYandexLemmaGetter(const TYandexLemma& lemma)
            : Lemma(lemma)
        {
        }
        const wchar16* GetLemmaText() const {
            return Lemma.LemmaText;
        }
        ui32 GetRuleId() const {
            return Lemma.RuleId;
        }

    private:
        const TYandexLemma& Lemma;
    };

    class TYandexWordformSetter {
    private:
        const TYandexWordformSetter& operator=(const TYandexWordformSetter&);

    public:
        TYandexWordformSetter(TYandexWordform& form)
            : Form(form)
        {
        }
        void SetNormalizedForm(const wchar16* form, size_t formLen, ELanguage language, size_t stemLen, size_t prefixLen, size_t suffixLen);
        void SetNormalizedForm(const TUtf16String& form, ELanguage language, size_t stemLen, size_t prefixLen, size_t suffixLen);
        void AddSuffix(const wchar16* Suffix, size_t len);
        void SetStemGr(const char* stemGr) {
            Form.StemGram = stemGr;
        }
        size_t AddFlexGr(const char* flexGr) {
            Form.FlexGram.push_back(flexGr);
            return Form.FlexGram.size();
        }
        void SetWeight(double weight) {
            Form.Weight = weight;
        }
        void Clear();

    private:
        TYandexWordform& Form;
    };

    inline ui32 GetRuleId(const TYandexLemma& lemma) {
        return TYandexLemmaGetter(lemma).GetRuleId();
    }

    inline NLemmer::TLanguageQualityAccept QualitySetByMaxLevel(NLemmer::EAccept accept) {
        using namespace NLemmer;
        TLanguageQualityAccept ret;
        switch (accept) {
            default: // AccTranslit etc
            case AccFoundling:
                ret.Set(AccFoundling);
                [[fallthrough]]; // Compiler happy. Please fix it!
            case AccBastard:
                ret.Set(AccBastard);
                [[fallthrough]]; // Compiler happy. Please fix it!
            case AccSob:
                ret.Set(AccSob);
                [[fallthrough]]; // Compiler happy. Please fix it!
            case AccDictionary:
                ret.Set(AccDictionary);
                break;
        }
        return ret;
    }
}
