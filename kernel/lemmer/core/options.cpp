#include "lemmeraux.h"
#include "options.h"

namespace NLemmer {
    const TLangMask TAnalyzeWordOpt::DefaultAcceptDictionary = ~TLangMask();
    const TLangMask TAnalyzeWordOpt::DefaultAcceptSob = ~TLangMask();
    const TLangMask TAnalyzeWordOpt::DefaultAcceptBastard = ~TLangMask();
    const TLangMask TAnalyzeWordOpt::DefaultAcceptFoundling = ~TLangMask();
    const TLangMask TAnalyzeWordOpt::DefaultAcceptFromEnglish = TLangMask();
    const TLangMask TAnalyzeWordOpt::DefaultAcceptTranslit = TLangMask();
    const TLangMask TAnalyzeWordOpt::DefaultGenerateQuasiBastards = TLangMask(LANG_KAZ) | TLangMask(LANG_TUR);

    const TAnalyzeWordOpt& TAnalyzeWordOpt::IndexerOpt() {
        struct TB {
            TAnalyzeWordOpt Res;
            TB()
                : Res()
            {
                Res.AcceptDictionary = NLanguageMasks::LemmasInIndex();
                Res.AcceptBastard = ~NLanguageMasks::NoBastardsInSearch();
                Res.AcceptSob = Res.AcceptBastard;
                Res.Multitoken = MtnSplitAllPossible;
                Res.ReturnFoundlingAnyway = true;
                Res.ResetLemmaToForm = true;
            }
        } static const builder;
        return builder.Res;
    }

    const TAnalyzeWordOpt& TAnalyzeWordOpt::IndexerLemmatizeAllOpt() {
        struct TB {
            TAnalyzeWordOpt Res;
            TB()
                : Res()
            {
                Res.AcceptBastard = ~NLanguageMasks::NoBastardsInSearch();
                Res.Multitoken = MtnSplitAllPossible;
            }
        } static const builder;
        return builder.Res;
    }

    const TAnalyzeWordOpt& TAnalyzeWordOpt::DefaultLemmerTestOpt() {
        struct TB {
            TAnalyzeWordOpt Res;
            TB()
                : Res()
            {
                Res.Suffix = SfxOnly;
                Res.Multitoken = MtnSplitAllPossible;
            }
        } static const builder;
        return builder.Res;
    }

    TTranslitOpt TAnalyzeWordOpt::GetTranslitOptions(ELanguage lang) const {
        TTranslitOpt opt;

        if (!AcceptDictionary.SafeTest(lang))
            opt.ExactForms.Set(AccDictionary);
        if (!AcceptSob.SafeTest(lang))
            opt.ExactForms.Set(AccSob);
        if (!AcceptBastard.SafeTest(lang))
            opt.ExactForms.Set(AccBastard);
        if (!AcceptFoundling.SafeTest(lang))
            opt.ExactForms.Set(AccFoundling);

        opt.NeedGramm = NeededGramm;
        return opt;
    }

    TRecognizeOpt::TRecognizeOpt(NLemmer::EAccept accept, const char* requiredGrammar, bool useFixList, bool generateQuasiBastards)
        : Accept(NLemmerAux::QualitySetByMaxLevel(accept))
        , RequiredGrammar(requiredGrammar)
        , UseFixList(useFixList)
        , GenerateQuasiBastards(generateQuasiBastards)
    {
    }

    TRecognizeOpt::TRecognizeOpt(size_t maxLemmasToReturn, NLemmer::TLanguageQualityAccept accept, const char* requiredGrammar, bool useFixList, bool generateQuasiBastards)
        : MaxLemmasToReturn(maxLemmasToReturn)
        , Accept(accept)
        , RequiredGrammar(requiredGrammar)
        , UseFixList(useFixList)
        , GenerateQuasiBastards(generateQuasiBastards)
    {
    }

    TRecognizeOpt::TRecognizeOpt(const NLemmer::TAnalyzeWordOpt& analyzeOpt, NLemmer::TLanguageQualityAccept accept)
        : Accept(accept)
        , RequiredGrammar(analyzeOpt.NeededGramm)
        , UseFixList(analyzeOpt.UseFixList)
        , SkipNormalization(analyzeOpt.SkipNormalization)
        , SkipAdvancedNormaliation(analyzeOpt.SkipAdvancedNormaliation)
        , GenerateAllBastards(analyzeOpt.GenerateAllBastards)
        , LastNonRareRuleId(analyzeOpt.LastNonRareRuleId)
        , AllowDeprecated(analyzeOpt.AllowDeprecated)
        , AllowEmptyLemma(analyzeOpt.AllowEmptyLemma)
    {
    }
}
