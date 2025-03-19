#include "analysis.h"
#include "mn_sse.h"
#include <kernel/factor_slices/factor_domain.h>
#include <kernel/factor_storage/factors_reader.h>
#include <kernel/factor_storage/factor_storage.h>

namespace NMatrixnet {

    TVector<float> GetFactorInfluenceRatings(const TMnSseInfo& model,
            TConstArrayRef<float> baseline,
            TConstArrayRef<float> test,
            const TInfluenceCalcOptions& option
    ) {
        TTreeExplanationParams cfg;
        cfg.Sort = false;
        TTreeExplanations explBaseline, explTest;
        Y_ENSURE(baseline.size() >= model.GetNumFeats(), "baseline features size != model features size\n");
        Y_ENSURE(test.size() >= model.GetNumFeats(), "test features size != model features size\n");
        model.ExplainTrees(baseline.begin(), cfg, explBaseline);
        model.ExplainTrees(test.begin(), cfg, explTest);
        TVector<float> result(baseline.size(), 0);
        for (size_t i : xrange(explBaseline.size())) {
            if (explBaseline[i]->Value != explTest[i]->Value) {
                const TVector<TTreeExplanation::TMxBinFeature>& condBaseline = explBaseline[i]->Conditions;
                const TVector<TTreeExplanation::TMxBinFeature>& condTest = explTest[i]->Conditions;
                for (size_t j : xrange(condBaseline.size())) {
                    const TTreeExplanation::TMxBinFeature& binFeatureBaseline = condBaseline[j];
                    const TTreeExplanation::TMxBinFeature& binFeatureTest = condTest[j];
                    if (binFeatureBaseline.Value != binFeatureTest.Value) {
                        Y_ASSERT(baseline[binFeatureBaseline.Factor] != test[binFeatureBaseline.Factor]);
                        if (option.UseAbsoluteInfluence) {
                            result[binFeatureBaseline.Factor] += std::abs(
                                    explBaseline[i]->Value - explTest[i]->Value);
                        } else {
                            result[binFeatureBaseline.Factor] += explBaseline[i]->Value - explTest[i]->Value;
                        }
                    }
                }
            }
        }
        return result;
    }

    TVector<TVector<double>> GetLeaves(const TMnSseStatic& mn) {
        const TVector<TMultiData::TLeafData>& multiData = std::get<TMultiData>(mn.Leaves.Data).MultiData;
        Y_ENSURE(multiData.size() == 1);
        TVector<TVector<double>> result;
        size_t dataIdx = mn.Meta.GetSizeToCount(0);
        for (size_t depth = 1; depth < mn.Meta.SizeToCountSize; ++depth) {
            const size_t treesNum = mn.Meta.SizeToCount[depth];
            for (size_t treeIdx = 0; treeIdx < treesNum; ++treeIdx) {
                result.emplace_back(1 << depth);
                for (size_t leaf = 0; leaf < (1 << depth); ++leaf) {
                    result.back()[leaf] = (multiData[0].Data[dataIdx] ^ (1 << 31)) * multiData[0].Norm.DataScale;
                    ++dataIdx;
                }
            }
        }
        return result;
    }

    TVector<TVector<TMxBinFeature>> GetFactors(const TMnSseStaticMeta& meta) {
        Y_ENSURE(meta.MissedValueDirectionsSize == 0);
        TVector<size_t> cond2findex;
        for (size_t i = 0; i < meta.FeaturesSize; ++i) {
            const size_t factorIndex = meta.Features[i].Index;
            const size_t bordersNum  = meta.Features[i].Length;
            for (size_t borderIdx = 0; borderIdx < bordersNum; ++borderIdx) {
                cond2findex.push_back(factorIndex);
            }
        }

        TVector<TVector<TMxBinFeature>> result;
        size_t treeCondIdx = 0;
        for (size_t depth = 1; depth < meta.SizeToCountSize; ++depth) {
            const size_t treesNum = meta.SizeToCount[depth];
            for (size_t treeIdx = 0; treeIdx < treesNum; ++treeIdx) {
                result.emplace_back();
                for (size_t i = 0; i < depth; ++i) {
                    const size_t condIdx = meta.GetIndex(treeCondIdx++);
                    result.back().push_back({cond2findex[condIdx], meta.Values[condIdx]});
                }
            }
        }
        return result;
    }

}
