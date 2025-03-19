#include "multifeature_softmax.h"
#include "bundle.h"
#include "dcg_boost.h"
#include <kernel/extended_mx_calcer/interface/common.h>
#include <library/cpp/linear_regression/unimodal.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/maybe.h>
#include <util/string/cast.h>

using namespace NExtendedMx;
using namespace NExtendedMx::NMultiFeatureSoftmax;

namespace {
    template<typename TVectorType>
    TSafeVector<TFeature> LoadFeatures(const TVectorType &data) {
        TSafeVector<TFeature> result;
        result.reserve(data.Size());
        for (const auto &feature : data) {
            TFeature feat;
            feat.Name = feature.Name();
            feat.Values.reserve(feature.Values().Size());
            for (const auto &val : feature.Values()) {
                feat.Values.push_back(*val.Get());
            }
            result.push_back(feat);
        }
        return result;
    }

    TString CombinationToString(const TCombination &combination) {
        return JoinStrings(combination.begin(), combination.end(), ",");
    }

    template<typename TVectorType>
    TSafeVector<ui32> LoadCombination(const TVectorType& data) {
        TSafeVector<ui32> combination;
        combination.reserve(data.Size());
        for (const auto &val : data)
            combination.push_back(val);
        return combination;
    }

    template<typename TVectorType>
    TSafeVector<TSafeVector<ui32>> LoadCombinations(const TVectorType &data) {
        TSafeVector<TSafeVector<ui32>> result;
        result.reserve(data.Size());
        for (const auto &row : data) {
            result.push_back(LoadCombination(row));
        }
        return result;
    }

    template<typename TVectorType>
    TSafeVector<NSc::TValue> LoadNoPosition(const TVectorType &data) {
        TSafeVector<NSc::TValue> result;
        result.reserve(data.Size());
        for (const auto &feature : data) {
            result.push_back(*feature.Get());
        }
        return result;
    }

    TFactors CalcUpperFactors(
        TCalcContext& ctx, const float* blenderFactors,
        const size_t blenderFactorCount, const TRelevCalcers& subtargets,
        const TMultiFeatureParams& mfp
    ) {
        TFactors subFactors;
        FillSubtargetFactors(subFactors, blenderFactors, blenderFactorCount, mfp);
        DebugFactorsDump("features", subFactors, ctx);
        TCategoricalFactors categoricalSubFactors;
        FillSubtargetCategoricalFactors(categoricalSubFactors, mfp);
        DebugFactorsDump("catfeatures", categoricalSubFactors, ctx);
        TVector<TVector<double>> subResults;
        CalcMultiple(subtargets, subFactors, categoricalSubFactors, subResults);

        TFactors factors;
        factors.assign(1, TVector<float>());
        factors[0].reserve(subResults.size() * mfp.Combinations.size() + mfp.UseSourceFeaturesInUpperFml * blenderFactorCount);
        for (const auto& res : subResults) {
            factors[0].insert(factors[0].end(), res.begin(), res.end());
        }
        if (mfp.UseSourceFeaturesInUpperFml) {
            factors[0].insert(factors[0].end(), blenderFactors, blenderFactors + blenderFactorCount);
        }
        return factors;
    }

    void BanNotAvailableFeatures(TCalcContext& ctx, TVector<bool>& availableCombinations, const TMultiFeatureParams& mfp) {
        for (size_t featIdx = 0; featIdx < mfp.Features.size(); ++featIdx) {
            const auto& featName = mfp.Features[featIdx].Name;
            const auto& availibleValues = ctx.GetMeta().FeatureContext()[featName].AvailibleValues().GetRawValue()->GetDict();
            if (availibleValues.empty()) {
                continue;
            }
            for (size_t combIdx = 0; combIdx < mfp.Combinations.size(); ++combIdx) {
                const NSc::TValue& featValue = mfp.GetFeatureValue(combIdx, featIdx);
                const TString featValueStr = featValue.IsString() ? ToString(featValue.GetString()) : ToString(featValue);
                if (!availibleValues.contains(featValueStr) || !availibleValues.Get(featValueStr).IsTrue()) {
                    availableCombinations[combIdx] = false;
                    if (ctx.DbgLog().IsEnabled()) {
                        ctx.DbgLog() << "combination " << combIdx << " is disabled because "
                                     << featName << "=" << featValueStr << " not available\n";
                    }
                }
            }
        }
    }

    size_t GetFeatureIndex(const TMultiFeatureParams& mfp, const TStringBuf& name) {
        return FindIf(mfp.Features, [&](const TFeature& feat) {return feat.Name == name;}) - mfp.Features.begin();
    }

    void BanNotAvailableViewPlacePos(TCalcContext& ctx, TVector<bool>& availableCombinations, const TMultiFeatureParams& mfp) {
        TCalcContext::TMetaConstProto meta = ctx.GetMeta();
        const auto& availVtComb = meta.AvailableVTCombinations();
        if (availVtComb.IsNull()) {
            return;
        }
        const auto& mainPlace = FEAT_PLACE_MAIN;
        size_t posFeatureIdx = GetFeatureIndex(mfp, FEAT_POS);
        size_t viewTypeFeatureIdx = GetFeatureIndex(mfp, FEAT_VIEWTYPE);
        if (posFeatureIdx != mfp.Features.size() && viewTypeFeatureIdx != mfp.Features.size()) {
            size_t placeFeatureIdx = GetFeatureIndex(mfp, FEAT_PLACE);
            for (size_t combIdx = 0; combIdx < mfp.Combinations.size(); ++combIdx) {
                const auto& comb = mfp.Combinations[combIdx];
                const auto& place = (placeFeatureIdx != mfp.Features.size()) ? mfp.Features[placeFeatureIdx].Values[comb[placeFeatureIdx]].GetString() : mainPlace;
                const auto& viewType = mfp.Features[viewTypeFeatureIdx].Values[comb[viewTypeFeatureIdx]].GetString();
                const auto& pos = mfp.Features[posFeatureIdx].Values[comb[posFeatureIdx]].GetIntNumber();
                if (!CombinationAvailableInContext(meta.AvailableVTCombinations(), viewType, place, pos, ctx.DbgLog())) {
                    availableCombinations[combIdx] = false;
                }
            }
        } else {
            ctx.DbgLog() << "didnt ban any combinations because ViewType or Pos is not a result feature\n";
        }
    }

    void UnbanNoPosition(TVector<bool>& availableCombinations, const TMultiFeatureParams& mfp) {
        const auto& combinationIndex = mfp.GetNoPositionCombinationIdx();
        if (combinationIndex.Defined()) {
            availableCombinations[*combinationIndex] = true;
        }
    }

    TVector<bool> GetAvailableCombinations(TCalcContext& ctx, const TMultiFeatureParams& mfp) {
        TVector<bool> availableCombinations(mfp.Combinations.size(), true);
        BanNotAvailableFeatures(ctx, availableCombinations, mfp);
        BanNotAvailableViewPlacePos(ctx, availableCombinations, mfp);
        if (mfp.NoPositionAlwaysAvailable) {
            UnbanNoPosition(availableCombinations, mfp);
        }
        return availableCombinations;
    }

    size_t CalcBundleNumFeats(const TRelevCalcers& subtargets, const TMnMcCalcer& upper, const TMultiFeatureParams& mfp) {
        size_t numFeats = 0;
        const size_t fakeFeaturesCount = mfp.GetFakeFeaturesCount();
        for (const auto& target : subtargets) {
            const size_t targetNumFeats = target->GetNumFeats();
            Y_ENSURE(targetNumFeats >= fakeFeaturesCount, "bad subtarget feat number");
            numFeats = Max(numFeats, targetNumFeats - fakeFeaturesCount);
        }
        const size_t combNumFeats = subtargets.size() * mfp.Combinations.size();
        const size_t upperNumFeats = upper.GetNumFeats();
        // cannot use exact check, because GetNumFeats is not equal number of features in training pool (it is equal to max used feature index + 1)
        if (!mfp.UseSourceFeaturesInUpperFml) {
            Y_ENSURE(upperNumFeats <= combNumFeats,
                     "Upper feats number " << upperNumFeats << " more than combination feats number " << combNumFeats);
        }
        if (mfp.UseSourceFeaturesInUpperFml && upperNumFeats >= combNumFeats) {
            numFeats = Max(numFeats, upperNumFeats - combNumFeats);
        }
        return numFeats;
    }

    void FilterAvailableCombinations(const float* features, const size_t featuresCount, TVector<bool>& availableCombinations,
                const TMultiFeatureParams& multiFeatureParams, const TRelevCalcer* filter,
                const double threshold, const TMultiFeatureParams& filterParams, const TSafeVector<size_t>& filterPositions)
    {
        TFactors intFactors;
        FillSubtargetFactors(intFactors, features, featuresCount, filterParams);
        TCategoricalFactors categFactors;
        FillSubtargetCategoricalFactors(categFactors, filterParams);
        TVector<TVector<double>> results;
        CalcMultiple(TRelevCalcers(1, filter), intFactors, categFactors, results);
        for (size_t combIdx = 0; combIdx < multiFeatureParams.Combinations.size(); ++combIdx) {
            if (results[0][filterPositions[combIdx]] < threshold) {
                availableCombinations[combIdx] = false;
            }
        }
        if (multiFeatureParams.NoPositionAlwaysAvailable) {
            UnbanNoPosition(availableCombinations, multiFeatureParams);
        }
    }
}

TMultiFeatureParams::TMultiFeatureParams(TMultiFeatureWinLossMcInfoConstProto& bundleInfo)
    : Combinations(LoadCombinations(bundleInfo.AllowedCombinations()))
    , Features(LoadFeatures(bundleInfo.Features()))
    , NoPosition(LoadNoPosition(bundleInfo.NoPosition()))
{}

size_t TMultiFeatureParams::GetFakeFeaturesCount() const {
    size_t count = 0;
    if (FakeFeaturesMode & FFM_AS_INTEGER) {
        count += Features.size();
    }
    if (FakeFeaturesMode & FFM_AS_BINARY) {
        for (size_t featIdx = 0; featIdx < Features.size(); ++featIdx) {
            count += Features[featIdx].Values.size();
        }
    }
    return count;
}

size_t TMultiFeatureParams::GetFeatureCateg(size_t combIdx, size_t featIdx) const {
    return Combinations[combIdx][featIdx];
}

const NSc::TValue& TMultiFeatureParams::GetFeatureValue(size_t combIdx, size_t featIdx) const {
    return Features[featIdx].Values[GetFeatureCateg(combIdx, featIdx)];
}

TMaybe<size_t> TMultiFeatureParams::GetCombinationIdx(const TSafeVector<NSc::TValue>& combinationFeatureValues) const {
    for (size_t combIdx = 0; combIdx < Combinations.size(); ++combIdx) {
        bool isSoughtCombination = true;
        for (size_t featIdx = 0; featIdx < Features.size(); ++featIdx) {
            if (GetFeatureValue(combIdx, featIdx) != combinationFeatureValues[featIdx]) {
                isSoughtCombination = false;
                break;
            }
        }
        if (isSoughtCombination) {
            return combIdx;
        }
    }
    return {};
}

TMaybe<size_t> TMultiFeatureParams::GetNoPositionCombinationIdx() const {
    return GetCombinationIdx(NoPosition);
}

bool TMultiFeatureParams::HasPosFeature() const{
    return AnyOf(Features, [](const auto& feat) {return feat.Name == FEAT_POS;});
}

size_t TMultiFeatureParams::GetPosFeatIdx() const {
    return GetFeatIdx(FEAT_POS);
}

size_t TMultiFeatureParams::GetFeatIdx(const TStringBuf featureName) const {
    return FindIf(Features, [&featureName](const auto& feat) {return feat.Name == featureName;}) - Features.begin();
}

void TMultiFeatureParams::Validate() const {
    Y_ENSURE(FakeFeaturesMode & FFM_AS_INTEGER || FakeFeaturesMode & FFM_AS_BINARY, "unused result features");
    for (const auto& feature : Features) {
        Y_ENSURE(feature.Name, "required feature name");
    }
    Y_ENSURE(Combinations.size() < 100000, "too many combinations");
    for (const auto& comb : Combinations) {
        Y_ENSURE(
            comb.size() == Features.size(),
            TStringBuilder() << "number of features in Combinations[i] should be equal to number of Features: "
                             << comb.size() << " != " << Features.size()
        );
    }
    Y_ENSURE(NoPosition.size() == Features.size(),
             "number of features for default combination should be equal to number of Features");
}

void NExtendedMx::NMultiFeatureSoftmax::FillSubtargetFactors(
    TFactors& factors, const float* blenderFactors, const size_t blenderFactorCount, const TMultiFeatureParams& mfp)
{
    TVector<float> subFeatures(mfp.GetFakeFeaturesCount());
    subFeatures.insert(subFeatures.end(), blenderFactors, blenderFactors + blenderFactorCount);
    factors.assign(mfp.Combinations.size(), subFeatures);
    for (size_t combIdx = 0; combIdx < mfp.Combinations.size(); ++combIdx) {
        size_t offset = 0;
        for (size_t featIdx = 0; featIdx < mfp.Features.size(); ++featIdx) {
            const size_t featCateg = mfp.GetFeatureCateg(combIdx, featIdx);
            if (mfp.FakeFeaturesMode & FFM_AS_INTEGER) {
                const auto& featVal = mfp.GetFeatureValue(combIdx, featIdx);
                if (featVal.IsIntNumber()) {
                    factors[combIdx][offset] = static_cast<float>(featVal.GetIntNumber());
                } else {
                    factors[combIdx][offset] = static_cast<float>(featCateg);
                }
                ++offset;
            }
            if (mfp.FakeFeaturesMode & FFM_AS_BINARY) {
                factors[combIdx][offset + featCateg] = 1.0;
                offset += mfp.Features[featIdx].Values.size();
            }
        }
    }
}

void NExtendedMx::NMultiFeatureSoftmax::FillSubtargetCategoricalFactors(
    TCategoricalFactors& factors, const TMultiFeatureParams& mfp)
{
    if (mfp.FakeFeaturesMode & FFM_AS_CATEGORICAL) {
        TVector<TString> categoricalSubFeatures(mfp.Features.size());
        factors.assign(mfp.Combinations.size(), categoricalSubFeatures);
        for (size_t combIdx = 0; combIdx < mfp.Combinations.size(); ++combIdx) {
            for (size_t featIdx = 0; featIdx < mfp.Features.size(); ++featIdx) {
                const auto& featVal = mfp.GetFeatureValue(combIdx, featIdx);
                if (featVal.IsIntNumber()) {
                    factors[combIdx][featIdx] = ToString(featVal.GetIntNumber());
                } else {
                    factors[combIdx][featIdx] = featVal.GetString();
                }
            }
        }
    }
}


TMaybe<size_t> NExtendedMx::NMultiFeatureSoftmax::GetCategIdFromIntentResult(const NSc::TValue& intentResult) {
    const auto& featureResult = intentResult["FeatureResult"];
    const auto& bundleInfo = intentResult["BundleInfo"];
    if (bundleInfo.IsNull() || bundleInfo["Type"] != WINLOSS_MC_TYPE || featureResult.IsNull()) {
        return {};
    }
    TMultiFeatureWinLossMcInfoConstProto bundleInfoData(&bundleInfo["Data"]);
    TMultiFeatureParams multiFeatureParams(bundleInfoData);
    TSafeVector<NSc::TValue> combinationFeatureValues;
    combinationFeatureValues.reserve(multiFeatureParams.Features.size());
    for (const auto& feature: bundleInfoData.Features()) {
        combinationFeatureValues.push_back(featureResult[feature.Name()]["Result"]);
    }
    return multiFeatureParams.GetCombinationIdx(combinationFeatureValues);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMultiFeatureSoftmaxBundle - softmax multiclassification bundle with subtargets, additional features allowed (such as position + type of wizard)
//                              each subtarget has first K (+1) binary features - indicators of positions (+1 as integer value)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiFeatureSoftmaxBundle : public TBundleBase<TMultiFeatureSoftmaxConstProto> {
private:
    template<typename TVectorType>
    TRelevCalcers LoadSubtargets(const TVectorType& data, const ISharedFormulasAdapter* const formulasStoragePtr) {
        TRelevCalcers result;
        result.reserve(data.Size());
        for (const auto &target : data) {
            auto sub = LoadFormulaProto<TRelevCalcer>(target, formulasStoragePtr);
            Y_ENSURE(sub, "could not load one of subtargets");
            result.push_back(sub);
        }
        return result;
    }

    bool CalcFilter(const float* features) const {
        if (Filter == nullptr)
            return true;
        TVector<const float*> factors;
        factors.push_back(features);
        TVector<double> result;
        Filter->CalcRelevs(factors, result);
        return !result.empty() && result.front() >= FilterThreshold;
    }

    TCateg GetTopPosition(const TVector<bool>& availableCombinations, const TVector<double>& scores, TCalcContext& context) const {
        size_t posFeatureId = MultiFeatureParams.Features.size();
        for (size_t i = 0; i < MultiFeatureParams.Features.size(); ++i) {
            if (MultiFeatureParams.Features[i].Name == FEAT_POS) {
                posFeatureId = i;
                break;
            }
        }
        if (posFeatureId == MultiFeatureParams.Features.size()) {
            return NOT_SHOWN_CATEG;
        }

        size_t noShowIdx = UpperCategs.size();
        for (size_t idx = 0; idx < UpperCategs.size(); ++idx) {
            if (UpperCategs[idx] == NOT_SHOWN_CATEG) {
                noShowIdx = idx;
                break;
            }
        }
        const double noShowScore = noShowIdx < UpperCategs.size() ? scores[noShowIdx] : std::numeric_limits<double>::lowest();
        const double threshold = Scheme().Params().ShowScoreThreshold();

        size_t bestShowIdx = UpperCategs.size();
        size_t bestPos = UpperCategs.size();
        for (size_t idx = 0; idx < UpperCategs.size(); ++idx) {
            if (idx == noShowIdx) {
                continue;
            }

            if (!availableCombinations[UpperCategs[idx]]) {
                continue;
            }

            const size_t pos = MultiFeatureParams.GetFeatureValue(UpperCategs[idx], posFeatureId).GetIntNumber();
            if (pos < bestPos && scores[idx] > noShowScore + threshold) {
                bestPos = pos;
                bestShowIdx = idx;
            }
        }

        if (bestShowIdx == UpperCategs.size()) {
            return NOT_SHOWN_CATEG;
        }

        LogValue(context, "show_noshow_score_diff_for_top", scores[bestShowIdx] - noShowScore);
        return UpperCategs[bestShowIdx];
    }

    TCateg CalculateUpper(const TFactors& factors, const TVector<bool>& availableCombinations, TVector<double>& scores, TCalcContext &context) const {
        Y_ENSURE(availableCombinations.size() == MultiFeatureParams.Combinations.size(), "Bad available combinations size");
        scores.resize(UpperCategs.size());
        for (const auto& f: factors) {
            Y_ENSURE(
                f.size() >= Upper->GetNumFeats(),
                TStringBuilder() << "Not enough features for upper: real " << f.size() << ", required " << Upper->GetNumFeats()
            );
        }
        Upper->CalcCategoriesRanking(factors, scores.data());

        if (Scheme().ForceUnimodal()) {
            TVector<double> arguments(UpperCategs.size());
            for (size_t i = 0; i < UpperCategs.size(); ++i) {
                arguments[i] = UpperCategs[i] < 0 ? 15 : UpperCategs[i];
            }
            MakeUnimodal(scores, arguments);
        }

        if (Scheme().Params().TopPosInsteadOfMax()) {
            return GetTopPosition(availableCombinations, scores, context);
        }

        size_t noShowIdx = UpperCategs.size();
        size_t bestShowIdx = UpperCategs.size();
        for (size_t idx = 0; idx < UpperCategs.size(); ++idx) {
            if (UpperCategs[idx] == NOT_SHOWN_CATEG) {
                noShowIdx = idx;
                continue;
            }

            if (!availableCombinations[UpperCategs[idx]]) {
                continue;
            }

            if (bestShowIdx == UpperCategs.size() || scores[idx] > scores[bestShowIdx]) {
                bestShowIdx = idx;
            }
        }

        if (bestShowIdx == UpperCategs.size()) {
            return NOT_SHOWN_CATEG;
        }

        if (noShowIdx == UpperCategs.size()) {
            return UpperCategs[bestShowIdx];
        }


        const double noShowScore = scores[noShowIdx];
        const double bestShowScore = scores[bestShowIdx];
        const double threshold = Scheme().Params().ShowScoreThreshold();

        LogValue(context, "show_noshow_score_diff", bestShowScore - noShowScore);
        if (bestShowScore - noShowScore > threshold) {
            return UpperCategs[bestShowIdx];
        }

        return NOT_SHOWN_CATEG;
    }

public:
    TMultiFeatureSoftmaxBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme)
    {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Upper = LoadFormulaProto<TRelevCalcer, TMnMcCalcer>(Scheme().Upper(), formulasStoragePtr);
        Subtargets = LoadSubtargets(Scheme().Subtargets(), formulasStoragePtr);
        MultiFeatureParams.Features = LoadFeatures(Scheme().Params().Features());
        MultiFeatureParams.Combinations = LoadCombinations(Scheme().Params().AllowedCombinations());
        MultiFeatureParams.NoPosition = LoadNoPosition(Scheme().Params().NoPosition());
        MultiFeatureParams.UseSourceFeaturesInUpperFml = Scheme().Params().UseSourceFeaturesInUpperFml();
        if (Scheme().Params().SetFeaturesAsInteger()) {
            MultiFeatureParams.FakeFeaturesMode |= FFM_AS_INTEGER;
        }
        if (Scheme().Params().SetFeaturesAsBinary()) {
            MultiFeatureParams.FakeFeaturesMode |= FFM_AS_BINARY;
        }
        Filter = !Scheme().Filter().IsNull() ? LoadFormulaProto<TRelevCalcer>(Scheme().Filter(), formulasStoragePtr) : nullptr;
        FilterThreshold = Scheme().Params().FilterThreshold();

        Y_ENSURE(Upper, "expected mnmc model");
        Y_ENSURE(Upper->CategValues(), "no categories detected");
        for (const auto& v : Upper->CategValues()) {
            auto categ = static_cast<TCateg>(v);
            Y_ENSURE(categ == NOT_SHOWN_CATEG || (categ >= 0 && categ < MultiFeatureParams.Combinations.ysize()),
                     TStringBuilder() << "Bad category " << categ);
            UpperCategs.push_back(categ);
        }
        NumFeats = CalcBundleNumFeats(Subtargets, *Upper, MultiFeatureParams);
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext &context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "multifeatures softmax calculation started: " << Scheme().Upper().Name() << '\t' << GetAlias() << "\n";
        auto selectedCateg = NOT_SHOWN_CATEG;
        if (!CalcFilter(features)) {
            if (context.DbgLog().IsEnabled())
                context.DbgLog() << "Request hasn't pass the filter" << '\n';
        } else {
            const TVector<bool> availableCombinations = GetAvailableCombinations(context, MultiFeatureParams);
            TVector<double> scores;
            const TFactors factors = CalcUpperFactors(context, features, featuresCount, Subtargets, MultiFeatureParams);
            selectedCateg = CalculateUpper(factors, availableCombinations, scores, context);
            if (context.DbgLog().IsEnabled()) {
                context.DbgLog() << "categ values:   " << JoinSeq("\t", UpperCategs) << '\n';
                context.DbgLog() << "categ result:   " << JoinSeq("\t", scores) << '\n';
                context.DbgLog() << "selected categ: " << selectedCateg << '\n';
            }
        }
        for (size_t featureIdx = 0; featureIdx < MultiFeatureParams.Features.size(); ++featureIdx) {
            const TFeature& feature = MultiFeatureParams.Features[featureIdx];
            auto result = selectedCateg >= 0 ? MultiFeatureParams.GetFeatureValue(selectedCateg, featureIdx) : MultiFeatureParams.NoPosition[featureIdx];
            context.GetResult().FeatureResult()[feature.Name].Result()->CopyFrom(result);
            LogValue(context, FEATURE_LOG_PREF + feature.Name, result);
        }
        return selectedCateg;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TRelevCalcers Subtargets;
    TMultiFeatureParams MultiFeatureParams;
    TMnMcCalcer* Upper = nullptr;
    TVector<TCateg> UpperCategs;
    TRelevCalcer* Filter = nullptr;
    double FilterThreshold = 0;
    size_t NumFeats = 0;
};


struct TScores {
public:

    template <class TProtoDict>
    void DumpToProtoDict(TProtoDict& dict) const {
        auto&& allCategsScores = dict.AllCategScores();
        allCategsScores["inserted_categs"] = insertedCategs;
        allCategsScores["upper_surplus"] = realScores;
        allCategsScores["upper_pure_surplus"] = pureScores;
        allCategsScores["upper_pure_win"] = pureWin;
        allCategsScores["categs_probs_keys"] = categsProbsKeys;
        allCategsScores["categs_probs_values"] = categsProbsValues;
        if (!additionScores.empty()) {
            allCategsScores["addition_scores"] = additionScores;
        }
        if (!relevBoosts.empty()) {
            allCategsScores["relev_boosts"] = relevBoosts;
        }
        dict.Aux()["surplus_multiplier"] = surplusMultiplier;
    }

    template <class TProtoDict>
    void DumpToProtoDict(TProtoDict& dict, const TCateg categ) const {
        auto&& bestCategScores = dict.BestCategScores();
        bestCategScores["upper_surplus"] = realScores[categ];
        bestCategScores["upper_pure_surplus"] = pureScores[categ];
        bestCategScores["upper_pure_win"] = pureWin[categ];
        if (!additionScores.empty()) {
            bestCategScores["addition_scores"] = additionScores[categ];
        }
        if (!relevBoosts.empty()) {
            bestCategScores["relev_boosts"] = relevBoosts[categ];
        }
    }

    template <class TProtoDict>
    static TScores LoadFromProtoDict(TCalcContext& ctx, TProtoDict&& dict, const THashMap<TStringBuf, size_t>& subtargetIdxByName, const size_t categsCount) {
        TScores scores;
        auto&& allCategsScores = dict.AllCategScores();
        scores.insertedCategs.assign(allCategsScores["inserted_categs"].begin(), allCategsScores["inserted_categs"].end());
        scores.realScores.assign(allCategsScores["upper_surplus"].begin(), allCategsScores["upper_surplus"].end());
        scores.pureScores.assign(allCategsScores["upper_pure_surplus"].begin(), allCategsScores["upper_pure_surplus"].end());
        scores.pureWin.assign(allCategsScores["upper_pure_win"].begin(), allCategsScores["upper_pure_win"].end());
        scores.categsProbsKeys.assign(allCategsScores["categs_probs_keys"].begin(), allCategsScores["categs_probs_keys"].end());
        scores.categsProbsValues.assign(allCategsScores["categs_probs_values"].begin(), allCategsScores["categs_probs_values"].end());
        scores.additionScores.assign(allCategsScores["addition_scores"].begin(), allCategsScores["addition_scores"].end());
        scores.relevBoosts.assign(allCategsScores["relev_boosts"].begin(), allCategsScores["relev_boosts"].end());
        scores.subScores.resize(subtargetIdxByName.size() * categsCount);
        scores.surplusMultiplier = dict.Aux()["surplus_multiplier"]->GetNumber();

        for (const auto& el : allCategsScores) {
            TStringBuf subtargetName;
            if (const auto& key = el.Key(); key.AfterPrefix("sub_", subtargetName)) {
                ctx.DbgLog() << key;
                const auto& subScores = el.Value();
                for (size_t i = 0; i <  subScores.Size(); ++i) {
                    scores.subScores[subtargetIdxByName.at(subtargetName) * categsCount + i] = subScores[i];
                }
            }
        }

        return scores;
    }

    TVector<int> insertedCategs;
    TVector<double> realScores;
    TVector<double> pureScores;
    TVector<double> pureWin;
    TVector<double> withoutAdditionModel;
    TVector<double> categsProbsKeys;
    TVector<double> categsProbsValues;
    TVector<double> additionScores;
    TVector<double> relevBoosts;
    TVector<float> subScores;
    double surplusMultiplier;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMultiFeatureWinLossMultiClassBundle
//  * multiclassification bundle with subtargets
//  * classess - win or loss for every combinations, so class numbers is equal to 2 * combinations_size
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiFeatureWinLossMultiClassBundle : public TBundleBase<TMultiFeatureWinLossMultiClassConstProto> {
private:
    TString ExtractSubtargetName(const NSc::TValue& scheme) {
        TString fullName(scheme.Get("OriginalName").GetString());
        if (fullName.empty()) {
            fullName = scheme.Get("Name").GetString();
        }
        TStringBuf name(fullName);
        name.RNextTok('.');
        return ToString(name);
    }

    template<typename TVectorType>
    void LoadSubtargets(TRelevCalcers& calcers, TVector<TString>& names, const TVectorType& data,
                        const ISharedFormulasAdapter* const formulasStoragePtr)
    {
        calcers.clear();
        calcers.reserve(data.Size());
        names.clear();
        names.reserve(data.Size());
        for (const auto& target : data) {
            auto sub = LoadFormulaProto<TRelevCalcer>(target, formulasStoragePtr);
            Y_ENSURE(sub, "could not load one of subtargets");
            calcers.push_back(sub);
            names.push_back(ExtractSubtargetName(*target->GetRawValue()));
        }
    }

    double GetSurplusMultiplier(const TVector<double>& rawProbs, const TVector<float>& subScores) const {
        const size_t combinationsCount = MultiFeatureParams.Combinations.size();
        double sumWinClassProb = 0.;
        double sumWinExpectation = 0.;
        for (size_t shownCategIdx = 0; shownCategIdx < combinationsCount; ++shownCategIdx) {
            double categWinScore = subScores[combinationsCount * WinSubtargetIndex + shownCategIdx];
            sumWinExpectation += std::exp(categWinScore) / (1 + std::exp(categWinScore));
            sumWinClassProb += rawProbs[shownCategIdx];
        }
        return sumWinExpectation / Max(sumWinClassProb, 1e-6);
    }

    const TScores CalcFinalScores(
        const THashMap<TCateg, double>& categsProbs,
        const THashMap<TCateg, double>& relevBoosts,
        const THashMap<TCateg, double>& additionScores,
        const double surplusMultiplier,
        const TVector<float>& subscores,
        TDebug& dbgLog
    ) const {

        const size_t combinationsCount = MultiFeatureParams.Combinations.size();

        TVector<TCateg> shownCategs;
        TVector<double> winClassProbs;
        TVector<double> lossClassProbs;
        TVector<double> surplusEstimates;
        TVector<double> dcgEstimates;

        TScores scores;
        scores.realScores.resize(combinationsCount, ShowScoreThreshold);
        scores.pureScores.resize(combinationsCount, ShowScoreThreshold);
        scores.pureWin.resize(combinationsCount, 0.0);

        if (ApplyAdditionBoost) {
            scores.additionScores.resize(combinationsCount, 0.0);
            if (RemainNoShowPositionThreshold.Defined() || RemainPositionIfShow) {
                scores.withoutAdditionModel.resize(combinationsCount, ShowScoreThreshold);
            }
        }
        if (ApplyDCGBoost) {
            scores.relevBoosts.resize(combinationsCount, 0.0);
        }
        for (const auto& categ : UpperCategs) {
            if (categ >= LossCategShift) {
                continue;  // skip loss category, it will be processed on dual win category
            }
            shownCategs.push_back(categ);
            winClassProbs.push_back(categsProbs.at(categ));
            lossClassProbs.push_back(categsProbs.at(categ + LossCategShift));

            const TString combination = CombinationToString(MultiFeatureParams.Combinations.at(categ));
            double currClickMultiplier = ClickBoost;
            double currSurplusAddition = 0.f;
            const auto combinationBoost = BoostsByCombination.FindPtr(combination);
            if (combinationBoost) {
                currClickMultiplier = combinationBoost->ClickMultiplier;
                currSurplusAddition = combinationBoost->SurplusAddition;
            }
            const double relativeSurplus = (
                categsProbs.at(categ) * currClickMultiplier - categsProbs.at(categ + LossCategShift) + currSurplusAddition);
            const double relativePureSurplus = categsProbs.at(categ) - categsProbs.at(categ + LossCategShift);
            double categScore = surplusMultiplier * relativeSurplus;

            surplusEstimates.push_back(categScore);
            if (ApplyDCGBoost) {
                dcgEstimates.push_back(relevBoosts.at(categ));
                categScore = SurplusCoef * categScore + RelevDCGCoef * relevBoosts.at(categ);
                scores.relevBoosts[categ] = relevBoosts.at(categ);
            }
            if (ApplyAdditionBoost) {
                if (RemainNoShowPositionThreshold.Defined() || RemainPositionIfShow) {
                    scores.withoutAdditionModel[categ] = categScore;
                }
                categScore += AdditionScoreCoef * additionScores.at(categ);
                scores.additionScores[categ] = additionScores.at(categ);
            }

            scores.realScores[categ] = categScore;
            scores.pureWin[categ] = categsProbs.at(categ) * surplusMultiplier;
            scores.pureScores[categ] = relativePureSurplus * surplusMultiplier;
        }
        scores.surplusMultiplier = surplusMultiplier;
        scores.subScores = subscores;
        scores.insertedCategs = shownCategs;
        scores.categsProbsKeys.reserve(categsProbs.size());
        scores.categsProbsValues.reserve(categsProbs.size());
        for (const auto& [key, value] : categsProbs) {
            scores.categsProbsKeys.push_back(key);
            scores.categsProbsValues.push_back(value);
        }

        if (dbgLog.IsEnabled()) {
            dbgLog << "categ values:         " << JoinSeq("\t", shownCategs) << '\n';
            dbgLog << "Win classes probs:    " << JoinVectorIntoString(winClassProbs, "\t") << "\n";
            dbgLog << "Loss classes probs:   " << JoinVectorIntoString(lossClassProbs, "\t") << "\n";
            dbgLog << "surplus multiplier:   " << surplusMultiplier << "\n";
            dbgLog << "surplus estimates:    " << JoinVectorIntoString(surplusEstimates, "\t") << "\n";
            if (ApplyDCGBoost) {
                dbgLog << "dcg impact estimates: " << JoinVectorIntoString(dcgEstimates, "\t") << "\n";
            }
        }

        return scores;
    }

    int SelectBestAvailableCombinationIdx(const TScores& scores, const TVector<bool>& availableCombinations, TDebug& dbgLog) const {
        Y_ENSURE(availableCombinations.size() == MultiFeatureParams.Combinations.size(),
                 "Bad available combinations size");

        const int bestCombinationIdx = GetMaxAvailableIdxWithThreshold(UseUnshiftedSurplusForScoreThreshold ? scores.pureScores : scores.realScores,
                                                              PickMaxWinPosition ? scores.pureWin : scores.realScores,
                                                              availableCombinations,
                                                              ShowScoreThreshold,
                                                              PickMaxWinPosition ? 0 : ShowScoreThreshold);
        const double bestScore = bestCombinationIdx == NOT_SHOWN_CATEG ? ShowScoreThreshold : scores.realScores[bestCombinationIdx];

        if (dbgLog.IsEnabled()) {
            dbgLog << "combinations result:         " << JoinVectorIntoString(scores.realScores, "\t") << "\n";
            dbgLog << "available combinations mask: " << JoinVectorIntoString(availableCombinations, "\t") << "\n";
            dbgLog << "selected combination:        " << bestCombinationIdx << "\n";
            dbgLog << "best score:                  " << bestScore << "\n";
        }
        return bestCombinationIdx;
    }

    const THashMap<TCateg, double> PrepareAndCalcCategRelevBoosts(TCalcContext& ctx, const float* blenderFactors, TDebug& dbgLog) const {
        double wizRelev;
        if (Scheme().RelevBoostParams().UseYandexTierRelevance()) {
            wizRelev = ctx.GetMeta().YandexTierRelevance();
        } else {
            wizRelev = WizRelevCalcer->DoCalcRelev(blenderFactors);
        }

        const auto& wizRelevTransform = Scheme().RelevBoostParams().WizRelevTransform();
        if (!wizRelevTransform.IsNull()) {
            wizRelev = ApplyResultTransform(wizRelev, wizRelevTransform);
        }

        TVector<double> docRelevs(DCGDocCount, 0.);
        for (size_t pos = 0; pos < DCGDocCount; ++pos) {
            docRelevs[pos] = *(blenderFactors + DocRelevsIndexes[pos]);
        }
        const auto& docRelevTransform = Scheme().RelevBoostParams().DocRelevTransform();
        if (!docRelevTransform.IsNull()) {
            ApplyResultTransform(docRelevs, docRelevTransform);
        }

        size_t bestPosByRelevance = MultiFeatureParams.NoPosition[PosFeatureIndex];
        THashMap<TCateg, double> categRelevDCGBoosts;
        CalcCategRelevBoosts(categRelevDCGBoosts, dbgLog,
                             wizRelev, docRelevs, bestPosByRelevance,
                             DCGDocCount, OrganicPosProbs, MultiFeatureParams,
                             UpperCategs, LossCategShift,
                             ViewTypeShift, GetFeatureIndex(MultiFeatureParams, FEAT_VIEWTYPE)
                             );

        ctx.GetResult().FeatureResult()["BestPosByRelevance"].Result()->CopyFrom(bestPosByRelevance);

        return categRelevDCGBoosts;
    }

    THashMap<TCateg, double> CalcCategAdditionScores(const float* blenderFactors, size_t blenderFactorCount, TDebug& dbgLog) const {

        TFactors AdditionModelFactors;
        FillSubtargetFactors(AdditionModelFactors, blenderFactors, blenderFactorCount, AdditionModelMultiFeatureParams);
        TCategoricalFactors AdditionModelCategoricalFactors;
        FillSubtargetCategoricalFactors(AdditionModelCategoricalFactors, AdditionModelMultiFeatureParams);
        TVector<TVector<double>> results;
        CalcMultiple(TRelevCalcers(1, AdditionModel), AdditionModelFactors, AdditionModelCategoricalFactors, results);
        if (dbgLog.IsEnabled()) {
            dbgLog << "Addition model scores: " << JoinVectorIntoString(results[0], "\t") << "\n";
        }
        if (AdditionModelSubtractCombinationIndex.Defined()) {
            const size_t subtractIndex = *AdditionModelSubtractCombinationIndex;
            for (auto& score : results[0]) {
                score -= results[0][subtractIndex];
            }
        }

        THashMap<TCateg, double> categAdditionScores(UpperCategs.size());
        for (const auto& categ : UpperCategs) {
            if (categ >= LossCategShift) {
                continue;  // iterate real categories like in CalculateUpper
            }

            TSafeVector<NSc::TValue> features;
            for (size_t addFeat = 0; addFeat < AdditionModelMultiFeatureParams.Features.size(); addFeat++) {
                bool featMatched = false;
                for (size_t mainFeat = 0; mainFeat < MultiFeatureParams.Features.size(); mainFeat++) {
                    if (AdditionModelMultiFeatureParams.Features[addFeat].Name == MultiFeatureParams.Features[mainFeat].Name) {
                        featMatched = true;
                        features.push_back(MultiFeatureParams.GetFeatureValue(categ, mainFeat));
                    }
                }
                Y_ENSURE(featMatched);
            }
            TMaybe<size_t> additionIdx = AdditionModelMultiFeatureParams.GetCombinationIdx(features);
            Y_ENSURE(additionIdx.Defined());
            categAdditionScores[categ] = results[0][additionIdx.GetRef()];
        }

        return categAdditionScores;
    }

    struct TCombinationBoost {
        double ClickMultiplier;
        double SurplusAddition;
    };

    template<typename TVectorType>
    THashMap<TString, TCombinationBoost> LoadBoostsByCombination(const TVectorType &data) {
        THashMap<TString, TCombinationBoost> result;
        if (!data.IsNull()) {
            for (const auto& combinationBoost : data) {
                TString combination = CombinationToString(LoadCombination(combinationBoost.Combination()));
                result[combination].ClickMultiplier = combinationBoost.ClickMultiplier();
                result[combination].SurplusAddition = combinationBoost.SurplusAddition();
            }
        }
        return result;
    }

    void LoadAdditionModelParams(const ISharedFormulasAdapter* const formulasStoragePtr) {
        const auto& AdditionModelParams = Scheme().AdditionModelParams();
        ApplyAdditionBoost = !AdditionModelParams.IsNull();
        if (!ApplyAdditionBoost) {
            return;
        }

        AdditionModel = LoadFormulaProto<TRelevCalcer>(AdditionModelParams.Model(), formulasStoragePtr);
        AdditionScoreCoef = AdditionModelParams.AdditionScoreCoef();
        SetMultiFeatureParamsForFactorsBuilding(AdditionModelMultiFeatureParams, AdditionModelParams);
        Y_ENSURE(!AdditionModelParams.SubtractNoPositionScore() ||
                    AdditionModelMultiFeatureParams.Features.size() == 1 &&
                    AdditionModelMultiFeatureParams.Features[0].Name == FEAT_POS,
                    "Currently, only Pos feature is supported for addition model with subtracting no show score");
        if (const auto threshold = AdditionModelParams.RemainNoShowPositionThreshold()) {
            RemainNoShowPositionThreshold = threshold;
        }
        RemainPositionIfShow = AdditionModelParams.RemainPositionIfShow();
        if (AdditionModelParams.SubtractNoPositionScore()) {
            TSafeVector<NSc::TValue> NoPositionCombination;
            // TODO: fix this while supporting other features
            NoPositionCombination.push_back(MultiFeatureParams.NoPosition[MultiFeatureParams.GetPosFeatIdx()]);
            AdditionModelSubtractCombinationIndex = AdditionModelMultiFeatureParams.GetCombinationIdx(NoPositionCombination);
            Y_ENSURE(AdditionModelSubtractCombinationIndex.Defined(),
                        "If SubtractNoPositionScore is true, NoPosition combination should present in addition model AllowedCombinations");
        }
    }

    void LoadPositionFilterParams(const ISharedFormulasAdapter* const formulasStoragePtr) {
        const auto& positionFilterParams = Scheme().PositionFilterParams();
        ApplyPositionFilter = !positionFilterParams.IsNull();
        if (ApplyPositionFilter) {
            PositionFilter = LoadFormulaProto<TRelevCalcer>(positionFilterParams.Model(), formulasStoragePtr);
            PositionFilterThreshold = positionFilterParams.Threshold();

            SetMultiFeatureParamsForFactorsBuilding(PositionFilterMultiFeatureParams, positionFilterParams);
            PositionFilterCombinationIndices.resize(MultiFeatureParams.Combinations.size());
            for (size_t combIdx = 0; combIdx < MultiFeatureParams.Combinations.size(); ++combIdx) {
                TSafeVector<NSc::TValue> filterCombinationValues;
                filterCombinationValues.reserve(PositionFilterMultiFeatureParams.Features.size());
                for (size_t filterFeatIdx = 0; filterFeatIdx < PositionFilterMultiFeatureParams.Features.size(); ++filterFeatIdx) {
                    for (size_t outerFeatIdx = 0; outerFeatIdx < MultiFeatureParams.Features.size(); ++outerFeatIdx) {
                        if (MultiFeatureParams.Features[outerFeatIdx].Name == PositionFilterMultiFeatureParams.Features[filterFeatIdx].Name) {
                            filterCombinationValues.push_back(MultiFeatureParams.GetFeatureValue(combIdx, outerFeatIdx));
                        }
                    }
                }
                TMaybe<size_t> combFilterIdx = PositionFilterMultiFeatureParams.GetCombinationIdx(filterCombinationValues);

                Y_ENSURE(!combFilterIdx.Empty(), "Some combination in formula has no appropriate combination in filter formula");
                PositionFilterCombinationIndices[combIdx] = *combFilterIdx;
            }
        }
    }

    void PassRawPredictions(TCalcContext& ctx, const TScores& upperScores, const TCateg bestCateg) const {
        if (!ctx.GetMeta().HasPassRawPredictions()) {
            return;
        }
        EPassRawPredictions passMode;
        if (!TryFromString(ctx.GetMeta().PassRawPredictions(), passMode)) {
            ctx.DbgLog() << "Unknown PassRawPredictions mode " << ctx.GetMeta().PassRawPredictions() << '\n';
            return;
        }

        auto res = ctx.GetResult().RawPredictions();

        const auto categCount = MultiFeatureParams.Combinations.size();
        if (passMode == EPassRawPredictions::Best && bestCateg != NOT_SHOWN_CATEG) {
            auto bestScores = res.BestCategScores();
            upperScores.DumpToProtoDict(res, bestCateg);
            for (size_t idx = 0; idx < SubtargetNames.size(); ++idx) {
                bestScores["sub_" + SubtargetNames[idx]] = upperScores.subScores.at(idx * categCount  + bestCateg);
            }
        } else if (passMode == EPassRawPredictions::All) {
            res.BestCateg() = bestCateg;
            upperScores.DumpToProtoDict(res);
            for (size_t idx = 0; idx < SubtargetNames.size(); ++idx) {
                const auto subScoreBegin = upperScores.subScores.begin() + idx * categCount;
                const auto subScoreEnd = subScoreBegin + categCount;
                Y_ENSURE(subScoreEnd <= upperScores.subScores.end());
                res.AllCategScores()["sub_" + SubtargetNames[idx]] = TVector<float>(subScoreBegin, subScoreEnd);
            }
        }
    }

public:

    using TRecalcParamsConstProto = TMultiFeatureWinLossMultiClassConstProto::TRecalcParamsConst;

    TMultiFeatureWinLossMultiClassBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme)
    {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Upper = LoadFormulaProto<TRelevCalcer, TMnMcCalcer>(Scheme().Upper(), formulasStoragePtr);
        LoadSubtargets(Subtargets, SubtargetNames, Scheme().Subtargets(), formulasStoragePtr);
        const auto& params = Scheme().Params();
        SetMultiFeatureParamsForFactorsBuilding(MultiFeatureParams, params);
        MultiFeatureParams.NoPosition = LoadNoPosition(params.NoPosition());
        MultiFeatureParams.UseSourceFeaturesInUpperFml = params.UseSourceFeaturesInUpperFml();
        MultiFeatureParams.NoPositionAlwaysAvailable = Scheme().Params().NoPositionAlwaysAvailable();
        ClickBoost = params.ClickBoost();
        BoostsByCombination = LoadBoostsByCombination(params.BoostsByCombination());
        ShowScoreThreshold = params.ShowScoreThreshold();
        UseUnshiftedSurplusForScoreThreshold = params.UseUnshiftedSurplusForScoreThreshold();
        PickMaxWinPosition = params.PickMaxWinPosition();
        WinSubtargetIndex = params.WinSubtargetIndex();
        Y_ENSURE(Upper, "expected mnmc model");
        Y_ENSURE(Upper->CategValues(), "no categories detected");
        LossCategShift = params.LossCategShift().IsNull() ? MultiFeatureParams.Combinations.ysize() : params.LossCategShift();
        Y_ENSURE(LossCategShift >= MultiFeatureParams.Combinations.ysize(), "Bad loss categ shift");
        for (const auto& v : Upper->CategValues()) {
            auto categ = static_cast<TCateg>(v);
            Y_ENSURE(categ >= 0 && categ < MultiFeatureParams.Combinations.ysize() + LossCategShift,
                     TStringBuilder() << "Bad category " << categ);
            UpperCategs.push_back(categ);
        }
        for (const auto& categ : UpperCategs) {
            const auto dualCateg = categ < LossCategShift ? categ + LossCategShift : categ - LossCategShift;
            Y_ENSURE(IsIn(UpperCategs, dualCateg), TStringBuilder() << "Can't find dual category for " << categ);
        }
        NumFeats = CalcBundleNumFeats(Subtargets, *Upper, MultiFeatureParams);

        const auto& relevParams = Scheme().RelevBoostParams();
        ApplyDCGBoost = !relevParams.IsNull() && MultiFeatureParams.HasPosFeature();
        if (ApplyDCGBoost) {
            SurplusCoef = relevParams.SurplusCoef();
            RelevDCGCoef = relevParams.RelevDCGCoef();
            DCGDocCount = relevParams.DCGDocCount();
            if (!params.HasWinSubtargetIndex()) {
                WinSubtargetIndex = relevParams.WinSubtargetIndex(); // deprecated
            }
            PosFeatureIndex = MultiFeatureParams.GetPosFeatIdx();
            DocRelevsIndexes.reserve(relevParams.DocRelevsIndexes().Size());
            for (size_t idx : relevParams.DocRelevsIndexes()) {
                DocRelevsIndexes.push_back(idx);
                NumFeats = Max(NumFeats, idx);
            }
            Y_ENSURE(DCGDocCount <= DocRelevsIndexes.size(),
                     TStringBuilder() << "In order to estimate dcg-top-"<< DCGDocCount << \
                     " relev predicts for at least " << DCGDocCount << " first wed documents are required.");
            WizRelevCalcer = LoadFormulaProto<TRelevCalcer> (relevParams.WizRelevCalcer(), formulasStoragePtr);
            NumFeats = Max(NumFeats, WizRelevCalcer->GetNumFeats());
            const auto& organicPosProbs = relevParams.OrganicPosProbs();
            const bool hasOrganicPosProbs = !organicPosProbs.IsNull();
            if (hasOrganicPosProbs) {
                for (const auto& categ : UpperCategs) {
                    if (categ >= LossCategShift) {
                        continue;
                    }
                    for (const auto& distr : organicPosProbs) {
                        if (distr.HasFeats()) {
                            bool matches = true;
                            for (auto it : distr.Feats()) {
                                if (MultiFeatureParams.GetFeatureValue(categ, MultiFeatureParams.GetFeatIdx(it.Key())) != *it.Value().GetRawValue()) {
                                    matches = false;
                                    break;
                                }
                            }
                            if (!matches) {
                                continue;
                            }
                        } else if (distr.HasWizPos()) {
                            const size_t wizPos = MultiFeatureParams.GetFeatureValue(categ, PosFeatureIndex).ForceIntNumber();
                            if (wizPos != distr.WizPos()) {
                                continue;
                            }
                        } else {
                            continue;
                        }

                        TVector<double> probs;
                        probs.reserve(DCGDocCount);
                        Y_ENSURE(distr.Probs().Size() >= DCGDocCount, "Wrong shape of OrganicPosProbs matrix.");
                        for (size_t organicPos = 0; organicPos < DCGDocCount; ++organicPos) {
                            probs.push_back(distr.Probs()[organicPos]);
                        }
                        OrganicPosProbs[categ].MainPosProbs = probs;
                        OrganicPosProbs[categ].RightProb = distr.RightProb();
                        OrganicPosProbs[categ].WizplaceProb = distr.WizplaceProb();
                        break;
                    }
                }
            }
            if (!relevParams.ViewTypeShift().Empty()) {
                size_t viewTypeFeatureIdx = GetFeatureIndex(MultiFeatureParams, FEAT_VIEWTYPE);
                for (const auto& it : relevParams.ViewTypeShift()) {
                    for (size_t i = 0; i < MultiFeatureParams.Features[viewTypeFeatureIdx].Values.size(); ++i) {
                        if (MultiFeatureParams.Features[viewTypeFeatureIdx].Values[i].ForceString() == it.Key()) {
                            ViewTypeShift[i] = std::make_pair(it.Value().Mult(), it.Value().Add());
                        }
                    }
                }
            }
            const size_t placeFeatureIndex = GetFeatureIndex(MultiFeatureParams, FEAT_PLACE);
            for (const auto& categ : UpperCategs) {
                if (categ >= LossCategShift) {
                    continue;
                }
                if (!hasOrganicPosProbs) {
                    if (placeFeatureIndex == MultiFeatureParams.Features.size() ||
                        MultiFeatureParams.GetFeatureValue(categ, placeFeatureIndex).GetString() == "Main")
                    {
                        const size_t wizPos = MultiFeatureParams.GetFeatureValue(categ, PosFeatureIndex).ForceIntNumber();
                        TVector<double> stubProbs(DCGDocCount, 0.0);
                        for (size_t organicPos = 0; organicPos < DCGDocCount; ++organicPos) {
                            stubProbs[organicPos] = (organicPos == wizPos ? 1.0 : 0.0);
                        }
                        OrganicPosProbs[categ].MainPosProbs = stubProbs;
                    } else {
                        TVector<double> stubProbs(DCGDocCount, 0.0);
                        OrganicPosProbs[categ].MainPosProbs = stubProbs;
                        TStringBuf place = MultiFeatureParams.GetFeatureValue(categ, placeFeatureIndex).GetString();
                        if (place == "Right") {
                            OrganicPosProbs[categ].RightProb = 1.0;
                        } else if (place == "Wizplace") {
                            OrganicPosProbs[categ].WizplaceProb = 1.0;
                        }
                    }
                }
                Y_ENSURE(OrganicPosProbs.contains(categ),
                         TStringBuilder() << "Don't have organic pos distribution for categ " << categ << ".");
            }
        }
        Y_ENSURE(WinSubtargetIndex < Subtargets.size());
        LoadAdditionModelParams(formulasStoragePtr);
        LoadPositionFilterParams(formulasStoragePtr);
        GetMeta().RegisterAttr(NMeta::YANDEX_TIER_RELEVANCE); // Unconditional till BLNDR-4560
    }

    template<typename TParams>
    void SetMultiFeatureParamsForFactorsBuilding(TMultiFeatureParams& multiFeatureParams, const TParams& params) {
        multiFeatureParams.Features = LoadFeatures(params.Features());
        multiFeatureParams.Combinations = LoadCombinations(params.AllowedCombinations());
        if (params.SetFeaturesAsInteger()) {
            multiFeatureParams.FakeFeaturesMode |= FFM_AS_INTEGER;
        }
        if (params.SetFeaturesAsBinary()) {
            multiFeatureParams.FakeFeaturesMode |= FFM_AS_BINARY;
        }
        if (params.SetFeaturesAsCategorical()) {
            multiFeatureParams.FakeFeaturesMode |= FFM_AS_CATEGORICAL;
        }
    }

    void WriteMultiPredictResult(const TVector<double>& scores, const TVector<bool>& availCombs, TCalcContext& context) const {
        context.GetResult().MultiPredict().Type() = "scored_categs";
        NSc::TValue* multiPredict = context.GetResult().MultiPredict().Data().GetRawValue();
        TScoredCategsProto scoredCategs(multiPredict);
        scoredCategs.Scores() = scores;
        scoredCategs.AvailableCombinations() = availCombs;
        scoredCategs.ShowScoreThreshold() = Scheme().Params().ShowScoreThreshold();
        scoredCategs.Features() = Scheme().Params().Features();
        scoredCategs.AllowedCombinations() = Scheme().Params().AllowedCombinations();
        scoredCategs.NoPosition() = Scheme().Params().NoPosition();
    }

    void WriteBundleInfo(TCalcContext& context) const {
        context.GetResult().BundleInfo().Type() = WINLOSS_MC_TYPE;
        NSc::TValue* bundleInfo = context.GetResult().BundleInfo().Data().GetRawValue();
        TMultiFeatureWinLossMcInfoProto winLossMcInfo(bundleInfo);
        winLossMcInfo.Features() = Scheme().Params().Features();
        winLossMcInfo.AllowedCombinations() = Scheme().Params().AllowedCombinations();
        winLossMcInfo.NoPosition() = Scheme().Params().NoPosition();
    }

    void RemainSurplusFormulaPositionHeuristics(
        int& selectedCombinationIdx,
        const TVector<bool>& availableCombinations,
        const TVector<double>& scoresWithoutAdditionModel,
        TDebug& dbgLog
    ) const {
        bool needCompareWithThreshold = RemainNoShowPositionThreshold.Defined() && selectedCombinationIdx != NOT_SHOWN_CATEG;
        if (needCompareWithThreshold || RemainPositionIfShow) {
            const int selectedCombinationWithoutAdditionModelIdx = GetMaxAvailableIdxWithThreshold(
                scoresWithoutAdditionModel, scoresWithoutAdditionModel, availableCombinations, ShowScoreThreshold, ShowScoreThreshold);
            if (dbgLog.IsEnabled()) {
                dbgLog << "combinations result without addition model:  " << JoinVectorIntoString(scoresWithoutAdditionModel, "\t") << "\n";
                dbgLog << "selected combination without addition model: " << selectedCombinationWithoutAdditionModelIdx << "\n";
            }
            if (selectedCombinationWithoutAdditionModelIdx == NOT_SHOWN_CATEG) {
                if (needCompareWithThreshold) {
                    const size_t chosenPosition = MultiFeatureParams.GetFeatureValue(selectedCombinationIdx, MultiFeatureParams.GetPosFeatIdx());
                    if (chosenPosition >= RemainNoShowPositionThreshold.GetRef()) {
                        selectedCombinationIdx = NOT_SHOWN_CATEG;
                        if (dbgLog.IsEnabled()) {
                            dbgLog << "Choose combination selected without addition model, because it is noshow ";
                            dbgLog << "and position with addition model " << chosenPosition << " is equal or ";
                            dbgLog << "below threshold " << RemainNoShowPositionThreshold.GetRef() << "\n";
                        }
                    }
                }
            } else if (RemainPositionIfShow) {
                selectedCombinationIdx = selectedCombinationWithoutAdditionModelIdx;
                if (dbgLog.IsEnabled()) {
                    dbgLog << "Choose combination selected without addition model, because it is show\n";
                }
            }
        }
    }

    double DoFinishWork(const TScores& scores, const TVector<bool>& availableCombinations, TCalcContext& context) const {
        int selectedCombinationIdx = SelectBestAvailableCombinationIdx(scores, availableCombinations, context.DbgLog());
        RemainSurplusFormulaPositionHeuristics(selectedCombinationIdx, availableCombinations,
                                               scores.withoutAdditionModel, context.DbgLog());

        const bool noShow = selectedCombinationIdx == NOT_SHOWN_CATEG;
        for (size_t featureIdx = 0; featureIdx < MultiFeatureParams.Features.size(); ++featureIdx) {
            const TFeature& feature = MultiFeatureParams.Features[featureIdx];
            const auto result = noShow ? MultiFeatureParams.NoPosition[featureIdx] : feature.Values[MultiFeatureParams.Combinations[selectedCombinationIdx][featureIdx]];
            context.GetResult().FeatureResult()[feature.Name].Result()->CopyFrom(result);
            LogValue(context, FEATURE_LOG_PREF + feature.Name, result);
        }

        PassRawPredictions(context, scores, selectedCombinationIdx);

        if (Scheme().Params().IsMultiShow()) {
            context.GetResult().IsMultiShow() = Scheme().Params().IsMultiShow();
        }

        if (context.GetMeta().IsMultiPredict()) {
            WriteMultiPredictResult(scores.realScores, availableCombinations, context);
        }

        if (context.GetMeta().AddBundleInfo()) {
            WriteBundleInfo(context);
        }

        return selectedCombinationIdx;
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << WINLOSS_MC_TYPE << " calculation started: " << Scheme().Upper().Name() << '\t' << GetAlias() << "\n";


        TVector<bool> availableCombinations = GetAvailableCombinations(context, MultiFeatureParams);
        if (ApplyPositionFilter) {
            FilterAvailableCombinations(features, featuresCount, availableCombinations, MultiFeatureParams,
                                        PositionFilter, PositionFilterThreshold, PositionFilterMultiFeatureParams, PositionFilterCombinationIndices);
        }

        const TFactors factors = CalcUpperFactors(context, features, featuresCount, Subtargets, MultiFeatureParams);

        TVector<double> rawProbs(UpperCategs.size(), 0.);
        Upper->CalcCategs(factors, rawProbs.data());

        THashMap<TCateg, double> categsProbs(UpperCategs.size());
        for (size_t i = 0; i < UpperCategs.size(); ++i) {
            categsProbs[UpperCategs[i]] = rawProbs[i];
        }
        const THashMap<TCateg, double> relevBoosts = ApplyDCGBoost
            ? PrepareAndCalcCategRelevBoosts(context, features, context.DbgLog())
            : THashMap<TCateg, double>{};

        const THashMap<TCateg, double> additionScores = ApplyAdditionBoost
            ? CalcCategAdditionScores(features, featuresCount, context.DbgLog())
            : THashMap<TCateg, double>{};

        const double surplusMultiplier = GetSurplusMultiplier(rawProbs, factors.at(0));

        const TScores scores = CalcFinalScores(
            categsProbs,
            relevBoosts,
            additionScores,
            surplusMultiplier,
            factors.at(0),
            context.DbgLog()
        );

        return DoFinishWork(scores, availableCombinations, context);
    }

    double RecalcRelevExtended(TCalcContext& ctx, const NSc::TValue& recalcParams) override {
        ctx.DbgLog() << WINLOSS_MC_TYPE << " recalculation started: " << Scheme().Upper().Name() << '\t' << GetAlias() << "\n";
        Y_ENSURE(ctx.GetResult().RawPredictions().HasAllCategScores(), "No AllCategScores needed for recalc in context");
        const TBasedOn<TRecalcParamsConstProto> recalcParamsProto(recalcParams);
        const auto& scheme = recalcParamsProto.Scheme();
        if (scheme.Params().HasShowScoreThreshold()) {
            ShowScoreThreshold = scheme.Params().ShowScoreThreshold();
        }
        if (scheme.Params().HasClickBoost()) {
            ClickBoost = scheme.Params().ClickBoost();
        }
        if (scheme.Params().HasBoostsByCombination()) {
            BoostsByCombination = LoadBoostsByCombination(scheme.Params().BoostsByCombination());
        }
        if (scheme.Params().HasPickMaxWinPosition()) {
            PickMaxWinPosition = scheme.Params().PickMaxWinPosition();
        }
        if (scheme.Params().HasUseUnshiftedSurplusForScoreThreshold()) {
            UseUnshiftedSurplusForScoreThreshold = scheme.Params().UseUnshiftedSurplusForScoreThreshold();
        }
        if (scheme.RelevBoostParams().HasSurplusCoef()) {
            SurplusCoef = scheme.RelevBoostParams().SurplusCoef();
        }
        if (scheme.RelevBoostParams().HasRelevDCGCoef()) {
            RelevDCGCoef = scheme.RelevBoostParams().RelevDCGCoef();
        }
        if (scheme.AdditionModelParams().HasAdditionScoreCoef()) {
            AdditionScoreCoef = scheme.AdditionModelParams().AdditionScoreCoef();
        }
        if (scheme.AdditionModelParams().HasRemainNoShowPositionThreshold()) {
            RemainNoShowPositionThreshold = scheme.AdditionModelParams().RemainNoShowPositionThreshold();
        }
        if (scheme.AdditionModelParams().HasRemainPositionIfShow()) {
            RemainPositionIfShow = scheme.AdditionModelParams().RemainPositionIfShow();
        }
        THashMap<TStringBuf, size_t> subtargetNamesToIdx(SubtargetNames.size());
        for (size_t i = 0; i < SubtargetNames.size(); ++i) {
            subtargetNamesToIdx[SubtargetNames[i]] = i;
        }
        TScores prevScores = TScores::LoadFromProtoDict(ctx, ctx.GetResult().RawPredictions(), subtargetNamesToIdx, MultiFeatureParams.Combinations.size());
        THashMap<TCateg, double> categsProbs;
        Y_ENSURE(prevScores.categsProbsKeys.size() == prevScores.categsProbsValues.size(), "categsProbs keys and values sizes do not match");
        for (size_t i = 0; i < prevScores.categsProbsKeys.size(); ++i) {
            categsProbs[prevScores.categsProbsKeys[i]] = prevScores.categsProbsValues[i];
        }

        THashMap<TCateg, double> relevBoosts;
        if (!prevScores.relevBoosts.empty()) {
            for (const auto& categ : prevScores.insertedCategs) {
                relevBoosts[categ] = prevScores.relevBoosts.at(categ);
            }
        }

        THashMap<TCateg, double> additionScores;
        if (!prevScores.additionScores.empty()) {
            for (const auto& categ : prevScores.insertedCategs) {
                additionScores[categ] = prevScores.additionScores.at(categ);
            }
        }

        const TScores scores = CalcFinalScores(
            categsProbs,
            relevBoosts,
            additionScores,
            prevScores.surplusMultiplier,
            prevScores.subScores,
            ctx.DbgLog()
        );

        return DoFinishWork(
            scores,
            GetAvailableCombinations(ctx, MultiFeatureParams),
            ctx
        );
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TRelevCalcers Subtargets;
    TVector<TString> SubtargetNames;
    TScores OldScores;

    TMultiFeatureParams MultiFeatureParams;
    TMnMcCalcer* Upper = nullptr;
    TVector<TCateg> UpperCategs;
    double ClickBoost = 1.f;
    THashMap<TString, TCombinationBoost> BoostsByCombination;
    double ShowScoreThreshold = 0.f;
    int LossCategShift = 0;
    size_t NumFeats = 0;

    bool ApplyAdditionBoost = false;
    TRelevCalcer* AdditionModel = nullptr;
    double AdditionScoreCoef = 0.f;
    TMaybe<size_t> RemainNoShowPositionThreshold;
    bool RemainPositionIfShow = false;
    TMultiFeatureParams AdditionModelMultiFeatureParams;
    TMaybe<size_t> AdditionModelSubtractCombinationIndex;

    bool ApplyDCGBoost = false;
    TRelevCalcer* WizRelevCalcer = nullptr;
    double SurplusCoef = 1.f;
    double RelevDCGCoef = 0.f;
    TVector<size_t> DocRelevsIndexes;
    size_t WinSubtargetIndex = 0;
    size_t PosFeatureIndex = 0;
    size_t DCGDocCount = 5;
    THashMap<TCateg, TOrganicPosProbs> OrganicPosProbs;
    THashMap<size_t, std::pair<double, double>> ViewTypeShift;

    //https://wiki.yandex-team.ru/users/wd28/UseUnshiftedSurplusForScoreThreshold-and-PickMaxWinPosition/
    bool PickMaxWinPosition = false;
    bool UseUnshiftedSurplusForScoreThreshold = false;

    bool ApplyPositionFilter = false;
    TRelevCalcer* PositionFilter = nullptr;
    double PositionFilterThreshold = 0.f;
    TMultiFeatureParams PositionFilterMultiFeatureParams;
    TSafeVector<size_t> PositionFilterCombinationIndices;
};


TExtendedCalculatorRegistrator<TMultiFeatureSoftmaxBundle> MultiFeatureSoftmaxRegistrator("multifeature_softmax");
TExtendedCalculatorRegistrator<TMultiFeatureWinLossMultiClassBundle> MultiFeatureWinLossMultiClassRegistrator(WINLOSS_MC_TYPE);
