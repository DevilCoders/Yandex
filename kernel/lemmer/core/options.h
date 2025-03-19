#pragma once

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/langs/langs.h>

namespace NLemmer {
    enum EAccept {
        AccFromEnglish = -2,
        AccTranslit = -1,
        AccDictionary = 0,
        AccSob,
        AccBastard,
        AccFoundling,
        AccInvalid
    };

    typedef TEnumBitSet<EAccept, AccFromEnglish, AccInvalid> TLanguageQualityAccept;

    enum TSuffixOpt {
        SfxBoth, // европа+ => европа, европа+
        SfxOnly, // европа+ => европа+
        SfxNo    // европа+ => европа
    };

    enum TMultitokenOpt {
        // н'ю-зв'язка => н'ю, зв'язка (in Ukrainian),
        // санкт-петербург => санкт-петербург, петербург (in Russian)
        // Йошкар-Ола => Йошкар-Ола (in Russian)
        MtnSplitAllPossible,
        // н'ю-зв'язка => н'ю-зв'язка (any language)
        MtnWholly,
        // н'ю-зв'язка => н, ю, зв, язка, зв'язка (in Ukrainian)
        // legacy
        MtnSplitAll
    };

    struct TTranslitOpt {
        TLanguageQualityAccept ExactForms;
        const char* NeedGramm;

        TTranslitOpt()
            : ExactForms()
            , NeedGramm("")
        {
        }

        static const TTranslitOpt& Default() {
            static const TTranslitOpt res;
            return res;
        }
    };

    struct TAnalyzeWordOpt {
        static const size_t DefMaxTokensInCompound = 3;
        static const size_t MaxTranslitLemmas = 4;
        static const TLangMask DefaultAcceptDictionary;
        static const TLangMask DefaultAcceptSob;
        static const TLangMask DefaultAcceptBastard;
        static const TLangMask DefaultAcceptFoundling;
        static const TLangMask DefaultAcceptFromEnglish;
        static const TLangMask DefaultAcceptTranslit;
        static const TLangMask DefaultGenerateQuasiBastards;

        size_t MaxTokensInCompound;
        const char* NeededGramm;
        TSuffixOpt Suffix;
        TMultitokenOpt Multitoken;
        TLangMask AcceptDictionary;
        TLangMask AcceptSob;
        TLangMask AcceptBastard;
        TLangMask AcceptFoundling;
        TLangMask AcceptFromEnglish;
        TLangMask AcceptTranslit;
        TLangMask GenerateQuasiBastards;
        bool UseFixList;
        bool ReturnFoundlingAnyway;
        bool ResetLemmaToForm;
        bool GenerateAllBastards;
        ui32 LastNonRareRuleId;
        bool AnalyzeWholeMultitoken;
        bool DoNotPrune;
        bool AllowDeprecated;
        bool AllowEmptyLemma;
        bool SkipNormalization;
        bool SkipAdvancedNormaliation;

        TAnalyzeWordOpt(const char* neededGramm = "", TSuffixOpt suffix = SfxBoth, TMultitokenOpt multitoken = MtnSplitAll, EAccept accept = AccFoundling)
            : MaxTokensInCompound(DefMaxTokensInCompound)
            , NeededGramm(neededGramm)
            , Suffix(suffix)
            , Multitoken(multitoken)
            , AcceptDictionary(DefaultAcceptDictionary)
            , AcceptSob(DefaultAcceptSob)
            , AcceptBastard(DefaultAcceptBastard)
            , AcceptFoundling(DefaultAcceptFoundling)
            , AcceptFromEnglish(DefaultAcceptFromEnglish)
            , AcceptTranslit(DefaultAcceptTranslit)
            , GenerateQuasiBastards(DefaultGenerateQuasiBastards)
            , UseFixList(true)
            , ReturnFoundlingAnyway(false)
            , ResetLemmaToForm(false)
            , GenerateAllBastards(false)
            , LastNonRareRuleId(0)
            , AnalyzeWholeMultitoken(false)
            , DoNotPrune(false)
            , AllowDeprecated(false)
            , AllowEmptyLemma(true)
            , SkipNormalization(false)
            , SkipAdvancedNormaliation(false)
        {
            switch (accept) {
                case AccDictionary:
                    AcceptSob = TLangMask();
                    [[fallthrough]];
                case AccSob:
                    AcceptBastard = TLangMask();
                    [[fallthrough]];
                case AccBastard:
                    AcceptFoundling = TLangMask();
                    [[fallthrough]];
                case AccFoundling:
                default: // AccTranslit etc
                    break;
            }
        }

        static const TAnalyzeWordOpt& IndexerOpt();
        static const TAnalyzeWordOpt& IndexerLemmatizeAllOpt();
        static const TAnalyzeWordOpt& DefaultLemmerTestOpt();

        TTranslitOpt GetTranslitOptions(ELanguage lang) const;
    };

    struct TRecognizeOpt {
        size_t MaxLemmasToReturn = static_cast<size_t>(-1);
        TLanguageQualityAccept Accept;
        const char* RequiredGrammar;
        bool UseFixList;
        bool SkipNormalization = false;
        bool SkipAdvancedNormaliation = false;
        bool SkipValidation = false;
        bool GenerateQuasiBastards;
        bool GenerateAllBastards = false;
        ui32 LastNonRareRuleId = 0;
        double MinAllowedProbability = 0.0;

        // если true, то в случае, когда подключено две версии морфологии для данного языка,
        // и вызывается Recognize для новой версии, то добавляются разборы из старой
        bool AllowDeprecated = false;

        // allow lemma text to be empty, but always
        // guarantee that number of lemmas is >= 1
        // partial rollback of r3043625
        // see SEARCHPRODINCIDENTS-2727
        //
        bool AllowEmptyLemma = true;

        TRecognizeOpt(EAccept = AccFoundling, const char* requiredGrammar = "", bool useFixList = true, bool generateQuasiBastards = false);
        TRecognizeOpt(size_t maxLemmasToReturn, TLanguageQualityAccept accept, const char* requiredGrammar, bool useFixList, bool generateQuasiBastards = false);
        TRecognizeOpt(const TAnalyzeWordOpt& analyzeOpt, TLanguageQualityAccept accept);
    };

}
