#include "calc_dssm.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <util/generic/string.h>

#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/facts/common/normalize_text.h>
#include <kernel/facts/dssm_applier/prepare_for_dssm.h>

namespace NSnippets {


// Returns 0 if the factor calculation was impossible or has failed
    float CalcDssmFactor(const TUtf16String& queryWide,
                         const TUtf16String& snippetWide,
                         const NNeuralNetApplier::TModel* dssmApplier,
                         ISnippetsCallback& callback,
                         const TString& name,
                         const TVector<TString>& dssmOutputs,
                         const TVector<TString>& sampleStrings) {
        static const TVector <TString> ANNOTATIONS = {"query", "answer"};

        float result = 0.0;
        bool success = false;

        if (dssmApplier != nullptr) {
            TAtomicSharedPtr <NNeuralNetApplier::ISample> sample = new NNeuralNetApplier::TSample(ANNOTATIONS, sampleStrings);

            TVector<float> output;
            dssmApplier->Apply(sample, dssmOutputs, output);

            if (output.size() == 1) {
                result = output[0];
                success = true;
            } else {
                if (callback.GetDebugOutput()) {
                    TString debugMessage = "Error: " + name + " factor calculation has failed";
                    callback.GetDebugOutput()->Print(true, "%s", debugMessage.data());
                }
                success = false;
            }
        }

        if (callback.GetDebugOutput()) {
            if (!success) {
                TString errorMessage = "Warning: " + name + " factor was not calculated";
                callback.GetDebugOutput()->Print(true, "%s", errorMessage.data());
            } else {
                TString debugMessage = " \nquery: " + WideToUTF8(queryWide) + " \nsnippet: " + WideToUTF8(snippetWide) + " \n" + name + " score: " + ToString(result) + "\n";
                callback.GetDebugOutput()->Print(true, "%s", debugMessage.data());
            }
        }

        return result;
    }


    void AdjustCandidatesDssm(TSnip& snip, const  TConfig& cfg, const TSentsMatchInfo& sentsMatchInfo, ISnippetsCallback& callback) {
        static const TString RU_FACT_SNIPPET_DSSM_NAME = "RuFactSnippet DSSM";
        static const TString TOMATO_DSSM_NAME = "TOMATO DSSM";
        static const TVector<TString> RU_FACT_SNIPPET_DSSM_OUTPUT{TString("score")};
        static const TVector<TString> TOMATO_DSSM_OUTPUT{TString("joint_afterdot")};
        const auto totalSnippetFactors = snip.Factors[NFactorSlices::EFactorSlice::SNIPPETS_MAIN].Size();
        if (totalSnippetFactors > A2_FQ_RU_FACT_SNIPPET_DSSM_FACTOID_SCORE) {
            TUtf16String snipCandidateTextWide = snip.GetRawTextWithEllipsis();
            if (!!snipCandidateTextWide) {
                const TUtf16String& queryTextWide = sentsMatchInfo.Query.OrigRequestText;
                const TVector<TString> ruFactSnippetValuesForDssm = {NFacts::PrepareTextForDssm(queryTextWide), NFacts::PrepareTextForDssm(snipCandidateTextWide)};
                snip.Factors[NFactorSlices::EFactorSlice::SNIPPETS_MAIN][A2_FQ_RU_FACT_SNIPPET_DSSM_FACTOID_SCORE] = CalcDssmFactor(queryTextWide, snipCandidateTextWide, cfg.GetRuFactSnippetDssmApplier(), callback, RU_FACT_SNIPPET_DSSM_NAME, RU_FACT_SNIPPET_DSSM_OUTPUT, ruFactSnippetValuesForDssm);
                if (!cfg.ExpFlagOn("disable_tomato_dssm") && totalSnippetFactors > A2_FQ_TOMATO_DSSM_FACTOID_SCORE) {
                    const TVector<TString> tomatoValuesForDssm = {
                            WideToUTF8(NUnstructuredFeatures::NormalizeText(queryTextWide)),
                            WideToUTF8(NUnstructuredFeatures::NormalizeText(snipCandidateTextWide))
                    };
                    snip.Factors[NFactorSlices::EFactorSlice::SNIPPETS_MAIN][A2_FQ_TOMATO_DSSM_FACTOID_SCORE] = CalcDssmFactor(queryTextWide, snipCandidateTextWide, cfg.GetTomatoDssmApplier(), callback, TOMATO_DSSM_NAME, TOMATO_DSSM_OUTPUT, tomatoValuesForDssm);
                }
            }
        }
    }

};
