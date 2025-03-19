#include "bundle.h"
#include "clickint.h"
#include "combinations.h"

#include <kernel/extended_mx_calcer/interface/common.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <util/generic/ymath.h>
#include <util/string/builder.h>


using namespace NExtendedMx;

template <typename T>
static T MaxElementIfPositiveElseFirst(const T& begin, const T& end, const double threshold) {
    if (begin != end) {
        auto maxIt = MaxElement(begin, end);
        return *maxIt >= threshold ? maxIt : begin;
    }
    return begin;
}

template <typename T>
static T FirstPositiveElseFirst(T begin, T end, const double threshold) {
    T first = begin;
    for (; begin != end; ++begin) {
        if ((*begin) > threshold) {
            return begin;
        }
    }
    return first;
}

template <typename T>
static size_t MaxIdxIfPositiveElseFirst(const T& begin, const T& end, const double threshold = 0.) {
    return MaxElementIfPositiveElseFirst(begin, end, threshold) - begin;
}

template <typename T>
static size_t FirstPositiveIdxElseFirst(const T& begin, const T& end, const double threshold = 0.) {
    return FirstPositiveElseFirst(begin, end, threshold) - begin;
}

template <typename TSrc, typename TDst>
static void Transpose(const TVector<TVector<TSrc>>& src, TVector<TVector<TDst>>& dst, const size_t colOffset = 0, const size_t colFeatures = 0) {
    dst.clear();
    if (src.empty()) {
        return;
    }
    const auto rows = src.size();
    const auto cols = src[0].size();
    dst.resize(cols, TVector<TDst>(rows + colOffset + colFeatures));
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            dst[j][i + colOffset] = src[i][j];
        }
    }
}

static void MultiCopyN(const TFactors& src, TFactors& dst, const size_t size) {
    Y_ENSURE(src.size() == dst.size(), "src and dst must have same length");
    if (size) {
        for (size_t i = 0; i < src.size(); ++i) {
            CopyN(src[i].data(), size, dst[i].data());
        }
    }
}

static void MultiCopyFeatures(TFactors& dst, const float* fPtr, const size_t fSize, const size_t dstOffset = 0) {
    if (fSize) {
        for (size_t i = 0; i < dst.size(); ++i) {
            CopyN(fPtr, fSize, dst[i].data() + dstOffset);
        }
    }
}

static size_t CalcFeatOffset(const TFeatureInfoConstProto& featInfo) {
    return featInfo.Binary() ? featInfo.Values().Size() : 1;
}

static void LogFeatureWeight(const TExtendedRelevCalcer& calcer, TCalcContext& context, const TStringBuf& featName, const double featStrength) {
    LogValue(calcer, context, FEATURE_WEIGHT_PREF + featName, featStrength);
}

namespace NExtendedMx {
    template <typename T>
    static void AddValue(TSet<T>& s, const T& t) {
        s.insert(t);
    }

    template <typename T>
    static void AddValue(TVector<T>& v, const T& t) {
        v.push_back(t);
    }

    void ExtractAdditionalFeatsResult(const TExtendedRelevCalcer* calcer, TAdditionalFeaturesConstProto additionalFeatures, TCalcContext& ctx, const TVector<float>& row, bool hasStep) {
        size_t offset = hasStep;
        for (const auto& learnedFeat : additionalFeatures) {
            Y_ENSURE(offset < row.size(), "ExtractAdditionalFeatsResult: out of range");
            TStringBuf result = "";
            if (!learnedFeat.Binary()) {
                result = learnedFeat.Values()[row[offset]];
            } else {
                const size_t end = learnedFeat.Values().Size() + offset;
                Y_ENSURE(end <= row.size(), "ExtractAdditionalFeatsResult: out of range");
                for (size_t i = offset; i < end; ++i) {
                    if (Abs(row[i] - 1) < std::numeric_limits<float>::epsilon()) {
                        result = learnedFeat.Values()[i - offset];
                    }
                }
            }
            const auto& name = learnedFeat.Name();
            ctx.GetResult().FeatureResult()[name].Result()->SetString(result);
            ctx.DbgLog() << "feat: " << name << '\t' << result << '\n';
            if (calcer) {
                LogValue(*calcer, ctx, FEATURE_LOG_PREF + name, result);
            }
            offset += CalcFeatOffset(learnedFeat);
        }
    }

    template<typename TCont>
    void IntersectFeats(TAdditionalFeaturesConstProto additionalFeatures, TCalcContextMetaProto ctxMeta, THashMap<TString, TCont>& validFeats) {
        validFeats.clear();
        for (const auto& learnedFeat : additionalFeatures) {
            const TStringBuf name = learnedFeat.Name();
            const auto& availibleValues = ctxMeta.FeatureContext()[name].AvailibleValues().GetRawValue()->GetDict();
            const auto& learnedVals = learnedFeat.Values();
            bool allAreValid = learnedVals.Size() == 1 && learnedVals[0].Get() == "*";
            if (allAreValid) {
                for (const auto& av : availibleValues) {
                    AddValue(validFeats[name], TString{av.first});
                }
            } else {
                for (const auto& v : learnedVals) {
                    if (allAreValid || availibleValues.contains(v)) {
                        AddValue(validFeats[name], TString{*v});
                    }
                }
            }
        }
    }

    static void InsertFeatsWithOffset(size_t totalRows, size_t offset, const float* fPtr, const size_t fSize, TFactors& dst) {
        dst.reserve(totalRows);
        dst.resize(1);
        auto& row = dst[0];
        row.reserve(offset + fSize);
        row.resize(offset, 0.f);
        row.insert(row.end(), fPtr, fPtr + fSize);
        dst.resize(totalRows, row);
    }

    void FillStepAndInsertFeatsWithOffset(size_t stepCount, size_t totalRows, size_t offset, const float* fPtr, const size_t fSize, TFactors& dst) {
        Y_ENSURE(stepCount, "step should be greater than zero");
        Y_ENSURE(totalRows % +stepCount == 0, "");
        Y_ENSURE(offset, "offset should be positive number");
        InsertFeatsWithOffset(totalRows, offset, fPtr, fSize, dst);
        size_t step = 0;
        for (auto& row : dst) {
            row[0] = step;
            if (++step == stepCount) {
                step = 0;
            }
        }
    }

    void ReplaceRepeatadly(TFactors& orig, const TFactors& patch, size_t inRowOffset, size_t rowsPerFeat) {
        Y_ENSURE(patch.size(), "patch should be non empty");
        Y_ENSURE(rowsPerFeat, "rows per feat should be non zero number");
        Y_ENSURE(orig.size() % (patch.size() * rowsPerFeat) == 0, "patch should be n-th part of orig");
        Y_ENSURE(patch[0].size() + inRowOffset < orig[0].size(), "too big patch");

        size_t idx = 0;
        for (size_t i = 0; i < orig.size(); ++i) {
            if (i % rowsPerFeat == 0 && i) {
                ++idx;
                idx %= patch.size();
            }
            const auto& prow = patch[idx];
            Copy(prow.begin(), prow.end(), orig[i].begin() + inRowOffset);
        }
    }

    void FillPredefinedFeatures(const TExtendedRelevCalcer* calcer, TFactors& factors, const float* fPtr, const size_t fSize, size_t stepCount,
                                TAdditionalFeaturesConstProto additionalFeatures, TCalcContext& context) {
        TValidFeatsSet validFeats;
        IntersectFeats(additionalFeatures, context.GetMeta(), validFeats);

        size_t totalRows = 1;
        size_t factorsOffset = 0;
        for (const auto& featInfo : additionalFeatures) {
            const auto& name = featInfo.Name();
            const auto* vals = validFeats.FindPtr(name);
            if (!vals || vals->empty()) {
                ythrow yexception() << "can not fill factors, additional feature is absent\n";
            }
            if (calcer) {
                LogValue(*calcer, context, FEATURE_AVAIL_PREF + name, JoinSeq("%", *vals));
            }
            totalRows *= vals->size();
            factorsOffset += CalcFeatOffset(featInfo);
        }
        size_t curOffset = 0;
        if (stepCount == NO_STEP) {
            InsertFeatsWithOffset(totalRows, factorsOffset, fPtr, fSize, factors);
        } else {
            totalRows *= stepCount;
            factorsOffset += 1;
            FillStepAndInsertFeatsWithOffset(stepCount, totalRows, factorsOffset, fPtr, fSize, factors);
            curOffset = 1;
        }
        context.DbgLog() << "total rows: " << factors.size() << '\n';
        size_t rowsPerFeat = totalRows;
        for (const auto& featInfo : additionalFeatures) {
            const auto& name = featInfo.Name();
            const auto& totalLearnedFeats = CalcFeatOffset(featInfo);

            TVector<TVector<float>> featVals;
            const auto& validVals = validFeats[name];
            featVals.reserve(validVals.size());
            const bool isBin = featInfo.Binary();
            rowsPerFeat /= validVals.size();
            for (size_t idx = 0; idx < featInfo.Values().Size(); ++idx) {
                if (validVals.contains(featInfo.Values()[idx])) {
                    featVals.emplace_back();
                    auto& back = featVals.back();
                    if (isBin) {
                        back.resize(totalLearnedFeats, 0.f);
                        back[idx] = 1.f;
                    } else {
                        back.push_back(idx);
                    }
                }
            }
            ReplaceRepeatadly(factors, featVals, curOffset, rowsPerFeat);
            curOffset += totalLearnedFeats;
        }
    }

    void RandomSelectAdditionalFeatures(const TExtendedRelevCalcer& calcer, const TAdditionalFeaturesConstProto& additionalFeats, TRandom& random, TCalcContext& context) {
        TValidFeatsVec validFeats;
        IntersectFeats(additionalFeats, context.GetMeta(), validFeats);
        for (auto& feat2vals : validFeats) {
            auto& vals = feat2vals.second;
            Sort(vals.begin(), vals.end());
            const auto& key = feat2vals.first;
            const auto& selected = vals[random.NextInt(vals.size())];
            context.GetResult().FeatureResult()[key].Result()->SetString(selected);
            LogValue(calcer, context, FEATURE_LOG_PREF + key, selected);
            LogValue(calcer, context, FEATURE_AVAIL_PREF + key, JoinSeq("%", vals));
        }
    }

} // NExtendedMx

static size_t CalcAdditionalOffset(TAdditionalFeaturesConstProto additionalFeatures) {
    size_t res = 0;
    for (const auto& featInfo : additionalFeatures) {
        res += CalcFeatOffset(featInfo);
    }
    return res;
}

template <typename TProto>
static double TransformStepAndLog(const TExtendedRelevCalcer& calcer, double step, const TProto& proto, TCalcContext& context) {
    context.DbgLog() << "selected_step " << step << '\n';
    LogValue(calcer, context, "selected_step", step);
    if (!proto.Step().IsNull()) {
        step *= proto.Step();
        LogValue(calcer, context, "step", proto.Step());
    }
    if (!proto.From().IsNull()) {
        step += proto.From();
        LogValue(calcer, context, "from", proto.From());
    }
    LogValue(calcer, context, "distrib", proto.ResultTransform().Distrib());
    context.DbgLog() << "before transform: " << step << '\n';
    step = ApplyResultTransform(step, proto.ResultTransform());
    context.DbgLog() << "after transform: " << step << '\n';
    LogValue(calcer, context, "result", step);
    return step;
}

static void InsertAndCalc(const TVector<float>& before, const float* fPtr, const size_t fSize, const NMatrixnet::IRelevCalcer& calcer, TVector<double>& results) {
    TVector<float> factors(1, 0.f);
    factors.insert(factors.end(), fPtr, fPtr + fSize);
    TFactors features(before.size(), factors);
    for (size_t i = 0; i < features.size(); ++i) {
        features[i][0] = before[i];
    }
    calcer.CalcRelevs(features, results);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TPositionalClickIntBundle - simple clickint calcer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TPositionalClickIntBundle : public TBundleBase<TPositionalClickIntConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
public:
    TPositionalClickIntBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Calcer = LoadFormulaProto<TCalcer>(Scheme().Fml(), formulasStoragePtr);
        Count = Scheme().Params().Count();
        FeatureName = Scheme().Params().FeatureName();
        DefaultPos = Scheme().Params().DefaultPos();
        NumFeats = NumFeatsWithOffset(Calcer->GetNumFeats(), CalcAdditionalOffset(Scheme().Params().AdditionalFeatures()) + 1);
        for (const auto& index : Scheme().Params().LogIndices()) {
            LogIndices.push_back(index);
        }
        Y_ENSURE(FeatureName, "required feature name");
        Y_ENSURE(Count && Count < 20, "count should be positive number and between first two pages");
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "positional clickint calculation started: " << Scheme().Fml().Name() << '\t' << GetAlias() << "\n";
        const auto& params = Scheme().Params();
        TFactors factors;
        FillPredefinedFeatures(this, factors, features, featuresCount, Count, params.AdditionalFeatures(), context);
        DebugFactorsDump("positional features", factors, context);
        TVector<double> result;
        Calcer->CalcRelevs(factors, result);
        const auto idxIt = MaxElement(result.begin(), result.end());
        const size_t idx = idxIt - result.begin();
        auto& row = factors[idx];

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "final result:\t" << JoinVectorIntoString(result, "\n") << '\n';
            context.DbgLog() << "selected row:\t" << JoinVectorIntoString(row, "\t") << '\n';
        }

        ExtractAdditionalFeatsResult(this, params.AdditionalFeatures(), context, row, true);
        size_t pos = *idxIt >= 0 ? static_cast<size_t>(row[0]) : DefaultPos;
        ProcessStepAsPos(*this, context, pos, FeatureName);

        if (!LogIndices.empty()) {
            TVector<double> loggedValues;
            loggedValues.resize(LogIndices.size(), 0.0);
            for (size_t i = 0; i < factors.size(); ++i) {
                size_t rowIdx = static_cast<size_t>(factors[i][0]);
                const auto logIdx = Find(LogIndices, rowIdx);
                if (logIdx != LogIndices.end()) {
                    loggedValues[*logIdx] = result[i];
                }
            }
            for (const double& valueToLog : loggedValues) {
                context.GetResult().FeatureResult()["InnerScores"].Result()->Push(valueToLog);
            }
        }
        return *idxIt;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* Calcer = nullptr;
    size_t Count = 10;
    TString FeatureName;
    size_t DefaultPos = 0;
    size_t NumFeats = 0;
    TVector<size_t> LogIndices;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TClickIntBundle - simple clickint calcer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TClickIntBundle : public TBundleBase<TClickIntConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
public:
    TClickIntBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Calcer = LoadFormulaProto<TCalcer>(Scheme().Fml(), formulasStoragePtr);
    }

private:
    double CalcClickInt(NExtendedMx::TCalcContext& context, const float* fPtr, const size_t fSize) const
    {
        static const size_t maxClickintSteps = 100;
        const auto& ciParams = Scheme().Params();
        const bool useThresholds = ciParams.UseThresholds();
        const auto& thresholds = ciParams.Thresholds();
        const size_t count = thresholds.Size();

        context.DbgLog() << "clickint calculation started: " << Scheme().Fml().Name() << '\t' << GetAlias() << '\n'
                         << "from, step, count: " << ciParams.From() << '\t' << ciParams.Step() << '\t' << count << '\n';

        if (count == 0 || count > maxClickintSteps) {
            return 0;
        }
        TVector<float> steps(count);
        double cur = 0.;
        double step = 1.;
        if (ciParams.TransformStepBeforeCalc()) {
            cur = ciParams.From();
            step = ciParams.Step();
        }
        for (auto& s : steps) {
            s = cur;
            cur += step;
        }

        TVector<double> results;
        InsertAndCalc(steps, fPtr, fSize, *Calcer, results);
        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "steps: " << JoinVectorIntoString(steps, "\t") << '\n';
            context.DbgLog() << "final result:\n" << JoinVectorIntoString(results, "\t") << '\n';
        }
        int i = static_cast<int>(results.size()) - 1;
        if (useThresholds) {
            for ( ; i > -1; --i) {
                context.DbgLog() << i << "|\t" << steps[i] << '\t' << results[i] << '\t' << thresholds[i] << '\n';
                if (results[i] > thresholds[i]) {
                    break;
                }
            }
            i = Max(i, 0);
        } else {
            i = MaxIdxIfPositiveElseFirst(results.begin(), results.end());
            if (context.DbgLog().IsEnabled()) {
                context.DbgLog() << "selected idx: " << i << '\n';
            }
        }
        if (ciParams.DumpPredicts()) {
            auto& rpp = context.GetResult().FeatureResult()["raw_predicts"]->GetRawValue()->SetArray();
            rpp.AppendAll(results.begin(), results.end());
        }
        return TransformStepAndLog(*this, i, ciParams, context);
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        return CalcClickInt(context, features, featuresCount);
    }

    size_t GetNumFeats() const override {
        return NumFeatsWithOffset(Calcer->GetNumFeats(), 1);
    }

private:
    TCalcer* Calcer = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TClickIntRandomBundle - random for clickint
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TClickIntRandom : public TBundleBase<TClickIntRandomConstProto> {
public:
    TClickIntRandom(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , AdditionalSeed(Scheme().Params().AdditionalSeed().Get())
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);
    }

    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& context) const override {
        context.DbgLog() << "clickint random calculation started: " << GetAlias() << '\n';
        const auto& seed = context.GetMeta().RandomSeed().Get() + AdditionalSeed;
        context.DbgLog() << "seed: " << seed << '\n';
        NExtendedMx::TRandom random(seed);
        const auto& params = Scheme().Params();
        double res = random.NextInt(params.Count());

        RandomSelectAdditionalFeatures(*this, params.AdditionalFeatures(), random, context);
        LogValue(context, "count", params.Count());
        return TransformStepAndLog(*this, res, params, context);
    }

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    const TString AdditionalSeed;
};

template <typename TProtoCont>
static void CopyAndPartialSumProbabilities(const TProtoCont& src, TVector<double>& dst) {
    dst.reserve(src.Size());
    Copy(src.begin(), src.end(), std::back_inserter(dst));
    Y_ENSURE(AllOf(dst, [](double v) { return v >= 0; }), "all probabilities should be greater or equal to zero");
    std::partial_sum(dst.begin(), dst.end(), dst.begin());
    Y_ENSURE(dst.back() > std::numeric_limits<double>::epsilon(), "too small probabilities, sum is: " + ToString(dst.back()));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TWeightLocalRandomBundle - generate random number by distrib
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TWeightLocalRandomBundle : public TBundleBase<TWeightRandomConstProto> {
    using TPartialSums = TVector<double>;

public:
    TWeightLocalRandomBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , AdditionalSeed(Scheme().Params().AdditionalSeed().Get())
      , FeatureName(Scheme().Params().FeatureName().Get())
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);

        const auto& params = Scheme().Params();
        const auto& probsParams = params.Probabilities();

        Y_ENSURE(probsParams.Size(), "empty probabilites");
        Y_ENSURE(params.Step() > std::numeric_limits<double>().epsilon());

        const size_t rowSize = probsParams[0].Size();
        for (const auto& row: probsParams) {
            Y_ENSURE(row.Size() == rowSize, "all probabilities should have equal size");
            ProbsByStep.emplace_back();
            CopyAndPartialSumProbabilities(row, ProbsByStep.back());
        }
    }

    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& context) const override {
        context.DbgLog() << "weight local random calculation started: " << GetAlias() << '\n';
        const auto& params = Scheme().Params();
        const auto& seed = TString{context.GetMeta().RandomSeed().Get()} + AdditionalSeed;
        context.DbgLog() << "seed:" << seed << '\n';
        NExtendedMx::TRandom random(seed);
        const auto& predictedWeightInfo = context.GetMeta().PredictedWeightInfo();
        Y_ENSURE(!predictedWeightInfo.IsNull(), "absent predicted weight");
        const double predictedWeight = predictedWeightInfo.Get();
        const double blenderInnerWeight = ApplyResultTransform(predictedWeight, params.IWTransform());
        const double stepSize = params.Step().Get();
        auto productionStep = static_cast<size_t>((blenderInnerWeight + 1e-5) / stepSize);
        productionStep = Min(productionStep, ProbsByStep.size() - 1);

        context.DbgLog() << "predictedWeight: " << predictedWeight << '\n';
        context.DbgLog() << "normWeight: " << blenderInnerWeight << '\n';
        context.DbgLog() << "productionStep: " << productionStep << '\n';

        const TPartialSums& sums = ProbsByStep[productionStep];
        const auto& weights = params.Probabilities()[productionStep];

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "selected weights: " << JoinSeq(" ", weights) << '\n';
            context.DbgLog() << "selected partial sums: " << JoinVectorIntoString(sums, " ") << '\n';
        }

        const size_t idx = random.Choice(sums);
        const double weight = params.Step() * idx;
        const double weightW = weights[idx] / sums.back();
        const double result = ApplyResultTransform(weight, params.ResultTransform());

        context.DbgLog() << "final weight: " << weight << '\n';
        context.DbgLog() << "result: " << result << '\n';

        if (FeatureName) {
            context.GetResult().FeatureResult()[FeatureName].Result()->SetNumber(result);
            LogValue(context, FEATURE_LOG_PREF + FeatureName, weight);
            LogValue(context, FEATURE_LOG_PREF + "step", params.Step().Get());
            LogValue(context, FEATURE_LOG_PREF + "idx", idx);
            LogValue(context, FEATURE_LOG_PREF + "prod_idx", productionStep);
            LogValue(context, FEATURE_LOG_PREF + "step_count", params.Probabilities().Size());
            LogFeatureWeight(*this, context, FeatureName, weightW);
        }

        return 0.;
    }

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    const TString AdditionalSeed;
    TVector<TPartialSums> ProbsByStep;
    const TString FeatureName;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TIdxRandomBundle - generate random number by distrib
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TIdxRandomBundle : public TBundleBase<TIdxRandomConstProto> {
    using TPartialSums = TVector<double>;

public:
    TIdxRandomBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , AdditionalSeed(Scheme().Params().AdditionalSeed().Get())
      , FeatureName(Scheme().Params().FeatureName().Get())
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);

        const auto& params = Scheme().Params();
        const auto& probsParams = params.Probabilities();
        const auto& probsByPosParams = params.ProbabilitiesByPos();
        const auto& idx2Pos = params.Idx2Pos();

        Y_ENSURE(probsParams.Size(), "empty probabilites");

        if (!probsByPosParams.Empty()) {
            GetMeta().RegisterAttr(NMeta::PREDICTED_POS_INFO);
        }

        CopyAndPartialSumProbabilities(probsParams, ProbsPartialSums);
        for (const auto& row: probsByPosParams) {
            Y_ENSURE(row.Size() == ProbsPartialSums.size(), "all probabilities should have equal size");
            ProbsByPosPartialSums.emplace_back();
            CopyAndPartialSumProbabilities(row, ProbsByPosPartialSums.back());
        }

        Y_ENSURE(idx2Pos.Empty() || idx2Pos.Size() == ProbsPartialSums.size(), "Idx2Pos should be empty or equal to probs partial sums");
        Idx2Pos.reserve(ProbsPartialSums.size());
        if (idx2Pos.Empty()) {
            for (size_t i = 0; i < ProbsPartialSums.size(); ++i) {
                Idx2Pos.push_back(i);
            }
        } else {
            Copy(idx2Pos.begin(), idx2Pos.end(), std::back_inserter(Idx2Pos));
        }
        DontShowIdx = FindIndex(Idx2Pos, params.DoNotShowPos().Get());
        HasNotShow = DontShowIdx < ProbsPartialSums.size();

        NeedMultiplierIfFiltered = !params.ShowProbaMultiplierIfFiltered().IsNull() && HasNotShow;
        if (NeedMultiplierIfFiltered) {
            GetMeta().RegisterAttr(NMeta::IS_FILTERED);
            const double multiplier = params.ShowProbaMultiplierIfFiltered();
            Y_ENSURE(multiplier > 0, "weight multiplier should be positive");
            CopyPartialSumsAndMultiplyDontShowWeight(ProbsPartialSums, ProbsPartialSumsIfFiltered, multiplier);
        }
    }

    double GetWeightFromPartialSums(const TPartialSums& partialSums, size_t index) const {
        return index == 0 ? partialSums[0] : partialSums[index] - partialSums[index - 1];
    }

    void CopyPartialSumsAndMultiplyDontShowWeight(const TPartialSums& src, TPartialSums& dst, double multiplier) {
        const double dontShowWeight = GetWeightFromPartialSums(src, DontShowIdx);
        const double totalWeight = src.back();
        const double showWeight = totalWeight - dontShowWeight;
        // newShowProba = showWeight / (showWeight + dontShowWeight * dontShowMultiplier) = showWeight * multiplier / totalWeight
        const double dontShowMultiplier = (totalWeight / multiplier - showWeight) / dontShowWeight;
        dst = src;
        for (size_t index = DontShowIdx; index < dst.size(); ++index) {
            dst[index] += dontShowWeight * (dontShowMultiplier - 1);
        }
    }

    const TPartialSums& ChoosePartialSums(TCalcContext& context, size_t predictedPos) const {
        if (NeedMultiplierIfFiltered && context.GetMeta().PredictedFeaturesInfo().IsFiltered()) {
            return ProbsPartialSumsIfFiltered;
        } else if (predictedPos < ProbsByPosPartialSums.size()) {
            return ProbsByPosPartialSums[predictedPos];
        } else {
            return ProbsPartialSums;
        }
    }

    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& context) const override {
        context.DbgLog() << "clickint random calculation started: " << GetAlias() << '\n';
        const auto& params = Scheme().Params();
        const auto& seed = TString{context.GetMeta().RandomSeed().Get()} + AdditionalSeed;
        context.DbgLog() << "seed:" << seed << '\n';
        NExtendedMx::TRandom random(seed);
        const auto& predictedPosInfo = context.GetMeta().PredictedPosInfo();
        const size_t predictedPos = predictedPosInfo.IsNull() ? params.DoNotShowPos().Get() : predictedPosInfo.Get();

        const TPartialSums& sums = ChoosePartialSums(context, predictedPos);
        const double dontShowWeight = HasNotShow ? GetWeightFromPartialSums(sums, DontShowIdx) : 0;
        const bool mayDontShow = HasNotShow && dontShowWeight > std::numeric_limits<double>::epsilon();

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "do not show idx: " << static_cast<int>(DontShowIdx) << '\n';
            context.DbgLog() << "may dont show: " << mayDontShow << '\n';
            context.DbgLog() << "selected partial sums: " << JoinVectorIntoString(sums, " ") << '\n';
        }

        const size_t idx = random.Choice(sums);
        const size_t pos = Idx2Pos[idx];
        const double posW = GetWeightFromPartialSums(sums, idx) / sums.back();

        if (FeatureName) {
            context.GetResult().FeatureResult()[FeatureName].Result()->SetIntNumber(pos);
            LogValue(context, FEATURE_LOG_PREF + FeatureName, pos);
            LogFeatureWeight(*this, context, FeatureName, posW);
            if (mayDontShow) {
                LogValue(context, "mbskipped", 1);
                LogValue(context, FEATURE_LOG_PREF + "noshow_p", dontShowWeight / sums.back());
            }
        }

        RandomSelectAdditionalFeatures(*this, params.AdditionalFeatures(), random, context);

        return pos;
    }

private:

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    const TString AdditionalSeed;
    bool NeedMultiplierIfFiltered;
    TPartialSums ProbsPartialSums;
    TPartialSums ProbsPartialSumsIfFiltered;
    TVector<TPartialSums> ProbsByPosPartialSums;
    TVector<size_t> Idx2Pos;
    const TString FeatureName;
    size_t DontShowIdx = 0;
    bool HasNotShow;
};

class TMultiFeatureLocalRandomBundle : public TBundleBase<TMultiFeatureLocalRandomConstProto> {
    using TParams = TMultiFeatureLocalRandomParamsConstProto;
    struct TCombination {
        TCombination(const TParams& params, size_t index)
            : Features(params.Features().Size())
        {
            // Features is set of values of index-th element of cartesian product of parama.Features()
            int mul = 1;
            for (size_t featIdx = 0; featIdx < Features.size(); ++featIdx) {
                size_t nVals = params.Features()[featIdx].Values().Size();
                Features[featIdx] = (index / mul) % nVals;
                mul *= nVals;
            }
        }

        TCombination(const TParams& params, const TCalcContextMetaConstProto& meta) {
            for (const auto& feat : params.Features()) {
                const auto& featValue = meta.PredictedFeaturesInfo().FeatureResult()[feat.Name()];
                if (!featValue.IsNull()) {
                    const auto& featValueArray = feat.Values().GetRawValue()->GetArray();
                    size_t featValueIdx = find(featValueArray.begin(), featValueArray.end(), *featValue.Result().Get()) - featValueArray.begin();
                    Y_ENSURE(featValueIdx != featValueArray.size(), TString("feat ") + feat.Name().Get() + " value is absent");
                    Features.push_back(featValueIdx);
                } else {
                    Y_ENSURE(feat.HasDefaultValueIdx(), TString("feat ") + feat.Name().Get() + " has no value and no default value");
                    Y_ENSURE(feat.DefaultValueIdx() < feat.Values().Size(), TString("feat ") + feat.Name().Get() + " has bad default index");
                    Features.push_back(feat.DefaultValueIdx());
                }
            }
        }

        size_t ToTransitionIndex(const TParams& params) const {
            // inverse to TCombination::TCombination(params, index)
            size_t curSize = 1;
            size_t idx = 0;
            for (size_t i = 0; i < Features.size(); ++i) {
                idx += Features[i] * curSize;
                curSize *= params.Features()[i].Values().Size();
            }
            return idx;
        }

        TVector<size_t> Features;
    };

    size_t GetFeatureIndex(const TStringBuf& featName) const {
        const auto& params = Scheme().Params();
        for (size_t featIdx = 0; featIdx < params.Features().Size(); ++featIdx) {
            if (params.Features()[featIdx].Name() == featName) {
                return featIdx;
            }
        }
        return params.Features().Size();
    }

    void BanNotAvailableFeatures(const TParams& params, TCalcContext& calcCtx, TVector<bool>& availableCombinations) const {
        TCalcContextMetaConstProto meta = calcCtx.GetMeta();
        for (size_t i = 0; i < Combinations.size(); ++i) {
            const auto& comb = Combinations[i];
            for (size_t featIdx = 0; featIdx < comb.Features.size(); ++featIdx) {
                const auto& featName = params.Features()[featIdx].Name();
                const auto& featValue = params.Features()[featIdx].Values()[comb.Features[featIdx]].AsString();
                if (!FeatureValueAvailableInContext(meta.FeatureContext(), featName, featValue, calcCtx.DbgLog())) {
                    availableCombinations[i] = false;
                    break;
                }
            }
        }
    }

    void BanNotAvailableViewPlacePos(const TParams& params, TCalcContext& ctx, TVector<bool>& availableCombinations) const {
        TCalcContextMetaConstProto meta = ctx.GetMeta();
        const TStringBuf mainPlace = FEAT_PLACE_MAIN;
        size_t posFeatureIdx = GetFeatureIndex(FEAT_POS);
        size_t viewTypeFeatureIdx = GetFeatureIndex(FEAT_VIEWTYPE);
        if (posFeatureIdx != params.Features().Size() && viewTypeFeatureIdx != params.Features().Size()) {
            size_t placeFeatureIdx = GetFeatureIndex(FEAT_PLACE);
            for (size_t combIdx = 0; combIdx < Combinations.size(); ++combIdx) {
                const auto& comb = Combinations[combIdx];
                const auto& place = (placeFeatureIdx != params.Features().Size()) ? params.Features()[placeFeatureIdx].Values()[comb.Features[placeFeatureIdx]].AsString() : mainPlace;
                const auto& viewType = params.Features()[viewTypeFeatureIdx].Values()[comb.Features[viewTypeFeatureIdx]].AsString();
                const auto& pos = params.Features()[posFeatureIdx].Values()[comb.Features[posFeatureIdx]]->GetIntNumber();
                availableCombinations[combIdx] = CombinationAvailableInContext(meta.AvailableVTCombinations(), viewType, place, pos, ctx.DbgLog());
            }
        } else {
            ctx.DbgLog() << "didnt ban any combinations because ViewType or Pos is not a result feature\n";
        }
    }

    TVector<bool> IntersectWithContext(const TParams& params, TCalcContext& calcCtx) const {
        TVector<bool> availableFeatIndices(Combinations.size(), true);
        BanNotAvailableFeatures(params, calcCtx, availableFeatIndices);
        BanNotAvailableViewPlacePos(params, calcCtx, availableFeatIndices);
        return availableFeatIndices;
    }

    void DumpTransitionMatrixIfDebug(const TParams& params, TCalcContext& calcCtx) const {
        if (!calcCtx.DbgLog().IsEnabled()) {
            return;
        }
        calcCtx.DbgLog() << "Transition matrix:\n";
        for (size_t i = 0; i < TransitionMatrix.size(); ++i) {
            const auto& comb = Combinations[i];
            for (size_t fi = 0; fi < comb.Features.size(); ++fi) {
                const auto& featVal = params.Features()[fi].Values()[comb.Features[fi]];
                if (featVal.IsPrimitive<int>()) {
                    calcCtx.DbgLog() << featVal.AsPrimitive<int>() << "\t";
                }
                if (featVal.IsString()) {
                    calcCtx.DbgLog() << featVal.AsString() << "\t";
                }
            }
            for (size_t j = 0; j < TransitionMatrix[i].size(); ++j) {
                calcCtx.DbgLog() << TransitionMatrix[i][j] << "\t";
            }
            calcCtx.DbgLog() << "\n";
        }
    }

public:
    TMultiFeatureLocalRandomBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , AdditionalSeed(Scheme().Params().AdditionalSeed().Get())
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);
        GetMeta().RegisterAttr(NMeta::FEATURES_INFO);
        GetMeta().RegisterAttr(NMeta::PREDICTED_FEATURES_INFO);
        const TParams& params = Scheme().Params();
        size_t featureCount = params.Features().Size();
        size_t featureProductSize = 1;
        for (const auto& feat : params.Features()) {
            featureProductSize *= feat.Values().Size();
        }
        Y_ENSURE(featureProductSize > 0, "all feats must have 1 or more values assigned to them");
        for (size_t i = 0; i < featureProductSize; ++i) {
            TCombination cur(params, i);
            Combinations.push_back(cur);
        }
        TVector<double> line(featureProductSize, 0.0);
        TransitionMatrix = TVector<TVector<double>>(featureProductSize, line);
        for (size_t i = 0; i < featureProductSize; ++i) {
            const auto& iComb = Combinations[i];
            for (size_t j = 0; j < featureProductSize; ++j) {
                const auto& jComb = Combinations[j];
                double transitionProbability = 1.0;
                for (size_t featIdx = 0; featIdx < featureCount; ++featIdx) {
                    size_t iFeatIdx = iComb.Features[featIdx];
                    size_t jFeatIdx = jComb.Features[featIdx];
                    transitionProbability *= params.Features()[featIdx].TransitionMatrix()[iFeatIdx][jFeatIdx];
                }
                TransitionMatrix[i][j] = transitionProbability;
            }
        }
    }

    double WriteAvailableFeatures(const TParams& params, TCalcContext& calcCtx, const TVector<bool>& availableCombinations) const {
        double extraNonPosWeight = 1.0;
        TVector<TStringBuf> featNames;
        const TCalcContextMetaConstProto& meta = calcCtx.GetMeta();
        for (const auto& feat : params.Features()) {
            featNames.push_back(feat.Name());
            TStringStream featValuesStr;
            size_t availableFeatures = 0;
            for (size_t i = 0; i < feat.Values().Size(); ++i) {
                if (!FeatureValueAvailableInContext(meta.FeatureContext(), feat.Name(), feat.Values()[i].AsString(), calcCtx.DbgLog())) {
                    continue;
                }
                if (availableFeatures > 0) {
                    featValuesStr << "%";
                }
                availableFeatures++;
                const auto& featVal = feat.Values()[i];
                if (featVal.IsPrimitive<int>()) {
                    featValuesStr << featVal.AsPrimitive<int>();
                }
                if (featVal.IsString()) {
                    featValuesStr << featVal.AsString();
                }
            }

            LogValue(calcCtx, FEATURE_AVAIL_PREF + feat.Name(), featValuesStr.Str());
            if (feat.Name() != FEAT_POS) {
                extraNonPosWeight *= availableFeatures;
            }
        }
        LogValue(calcCtx, FEATURE_AVAIL_PREF + "Features", JoinVectorIntoString(featNames, "%"));
        LogValue(calcCtx, FEATURE_AVAIL_PREF + "Combinations", JoinVectorIntoString(availableCombinations, "%"));
        return extraNonPosWeight;
    }

    void RewriteResultFeatures(TCalcContext& calcCtx, const TCombination& comb) const {
        const auto& params = Scheme().Params();
        for (size_t i = 0; i < params.Features().Size(); ++i) {
            const auto& featName = params.Features()[i].Name();
            const auto& featValue = params.Features()[i].Values()[comb.Features[i]];
            calcCtx.GetResult().FeatureResult()[featName].Result() = featValue;
            LogValue(calcCtx, FEATURE_LOG_PREF + featName, *featValue.GetRawValue());
        }
    }

    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& calcCtx) const override {
        const auto& params = Scheme().Params();
        DumpTransitionMatrixIfDebug(params, calcCtx);
        const TCombination predictedCombination(params, calcCtx.GetMeta());
        if (calcCtx.DbgLog().IsEnabled()) {
            calcCtx.DbgLog() << "predicted comb: " << JoinVectorIntoString(predictedCombination.Features, "\t") << "\n";
        }
        const size_t transitionProbsIndex = predictedCombination.ToTransitionIndex(params);
        const auto& availableCombinations = IntersectWithContext(params, calcCtx);
        double extraWeight = WriteAvailableFeatures(params, calcCtx, availableCombinations);
        if (calcCtx.DbgLog().IsEnabled()) {
            calcCtx.DbgLog() << "available combs: " << JoinVectorIntoString(availableCombinations, "\t") << "\n";
        }

        const auto& transitionProbabilies = TransitionMatrix[transitionProbsIndex];
        TVector<double> partialSums = {availableCombinations[0] ? transitionProbabilies.front() : 0};
        for (size_t i = 1; i < transitionProbabilies.size(); ++i) {
            partialSums.push_back(availableCombinations[i] ? partialSums.back() + transitionProbabilies[i] : partialSums.back());
        }
        const auto& seed = TString{calcCtx.GetMeta().RandomSeed().Get()} + AdditionalSeed;
        NExtendedMx::TRandom random(seed);
        const size_t choice = random.Choice(partialSums);
        const TCombination resultCombination(params, choice);
        if (calcCtx.DbgLog().IsEnabled()) {
            calcCtx.DbgLog() << "partial sums" << JoinVectorIntoString(partialSums, "\t") << "\n";
            calcCtx.DbgLog() << "selected " << choice << "\n";
            calcCtx.DbgLog() << "result combination " << JoinVectorIntoString(resultCombination.Features, "\t") << "\n";
        }
        // Correcting weight to account for blender random pool collector behaviour:
        // they divide pos weight by #of available feature values for each additional feature
        const double featureWeight = transitionProbabilies[choice] / partialSums.back() * extraWeight;
        LogFeatureWeight(*this, calcCtx, FEAT_POS, featureWeight);
        RewriteResultFeatures(calcCtx, resultCombination);
        return 0.0;
    }

    size_t GetNumFeats() const override {
        return 0;
    }
private:
    TString AdditionalSeed;
    TVector<TCombination> Combinations;
    TVector<TVector<double>> TransitionMatrix;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TViewPlacePosRandomBundle - generate a random combination of placement/position/viewtype
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TViewPlacePosRandomBundle : public TBundleBase<TViewPlacePosRandomConstProto> {
public:
    struct TShowItem {
        TStringBuf ViewType;
        TStringBuf Place;
        ui32 Pos;
        double Weight;

        static TShowItem CreateNoShow(const double weight) {
            return TShowItem{"NO_SHOW", "NO_SHOW", Max<ui32>(), weight};
        }

        bool IsNoShow() const {
            return ViewType == "NO_SHOW";
        }

        bool operator < (const TShowItem& rhs) const {
            return std::tie(ViewType, Place, Pos) < std::tie(rhs.ViewType, rhs.Place, rhs.Pos);
        }

        TString ToString() const {
            return ToTValue().ToJson();
        }

        NSc::TValue ToTValue() const {
            NSc::TValue ret;
            ret[FEAT_PLACE] = Place;
            ret[FEAT_POS] = Pos;
            ret[FEAT_VIEWTYPE] = ViewType;
            ret["Weight"] = Weight;
            return ret;
        }
    };

private:
    using TWeightedShowItems = THashMap<TString, THashMap<TString, TVector<double>>>;
    using TShowItems = TVector<TShowItem>;

public:
    TViewPlacePosRandomBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , AdditionalSeed(Scheme().Params().AdditionalSeed().Get())
      , MayNotShow(Scheme().Params().NoShowWeight() > 0)
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);
        GetMeta().RegisterAttr(NMeta::FEATURES_INFO);

        const auto& params = Scheme().Params();
        const auto& showWeights = params.ShowWeights();
        Y_ENSURE(params.NoShowWeight() >= 0, "no show weight should be greater or equal to zero");
        Init(showWeights);
    }

    const TVector<double>& GetWeights(const TStringBuf& viewType, const TStringBuf& place) const {
        if (const auto* place2weights = ShowItemWeights.FindPtr(viewType)) {
            if (const auto* weights = place2weights->FindPtr(place)) {
                return *weights;
            }
        }
        static const TVector<double> empty;
        return empty;
    }

    static void TryAppendToItem(TShowItems& items, const TStringBuf& viewType, const TStringBuf& place, const TVector<double>& weights, ui32 pos) {
        double weight = pos < weights.size() ? weights[pos] : -1;
        if (weight > 0) {
            items.push_back(TShowItem{viewType, place, pos, weight});
        }
    }

    void FillPositions(TShowItems& items, const TStringBuf& viewType, const TStringBuf& place, const NSc::TValue& nodeWithPoses) const  {
        const auto& weights = GetWeights(viewType, place);
        if (!weights) {
            return;
        }
        if (nodeWithPoses == "*") {
            for (ui32 idx = 0; idx < weights.size(); ++idx) {
                TryAppendToItem(items, viewType, place, weights, idx);
            }
        } else if (nodeWithPoses.IsArray()) {
            for (const auto& p : nodeWithPoses.GetArray()) {
                TryAppendToItem(items, viewType, place, weights, p.GetIntNumber());
            }
        } else {
            ythrow yexception() << "unknown node with positions: " << nodeWithPoses;
        }
    }

    TShowItems IntersectWithContext(TCalcContext& calcCtx) const {
        TCalcContextMetaConstProto meta = calcCtx.GetMeta();
        TShowItems items;
        const auto& availVTComb = calcCtx.GetMeta().AvailableVTCombinations();
        for (const auto& vt2place2pos : availVTComb) {
            const auto viewType = vt2place2pos.Key();
            if (!FeatureValueAvailableInContext(meta.FeatureContext(), FEAT_VIEWTYPE, viewType, calcCtx.DbgLog())) {
                continue;
            }
            const auto vtScheme = Scheme().Params().ShowWeights()[viewType];
            if (vtScheme.Empty()) {
                continue;
            }
            for (const auto& place2pos : vt2place2pos.Value()) {
                const auto place = place2pos.Key();
                if (!FeatureValueAvailableInContext(meta.FeatureContext(), FEAT_PLACE, TStringBuf(place), calcCtx.DbgLog())) {
                    continue;
                }
                const auto placeScheme = vtScheme[place];
                if (placeScheme.Empty()) {
                    continue;
                }
                FillPositions(items, viewType, place, *place2pos.Value().GetRawValue());
            }
        }
        return items;
    }

    TShowItems GetEditedShowItemsFromContext(TCalcContext& calcCtx) const {
        const auto& availVTComb = calcCtx.GetMeta().AvailableVTCombinations();
        TShowItems showItems = availVTComb.Empty() ? RootItems : IntersectWithContext(calcCtx);
        if (MayNotShow) {
            showItems.push_back(TShowItem::CreateNoShow(Scheme().Params().NoShowWeight()));
        }
        return showItems;
    }

    template <typename TProtoCont>
    void Init(const TProtoCont& showWeights) {
        for (const auto& viewType2place : showWeights) {
            const auto viewType = viewType2place.Key();
            for (const auto& place2pos : viewType2place.Value()) {
                const auto place = place2pos.Key();
                const auto& posWeights = place2pos.Value();
                for (ui32 pos = 0; pos < posWeights.Size(); ++pos) {
                    double weight = posWeights[pos];
                    Y_ENSURE(weight >= 0, "probability should be greater or equal to zero");
                    ShowItemWeights[viewType][place].push_back(weight);
                    if (weight > 0) {
                        RootItems.push_back(TShowItem{viewType, place, pos, weight});
                    }
                }
            }
        }
        Y_ENSURE(RootItems.size(), "no combinations with positive probability detected");
    }

    void DbgDumpCombinations(const TShowItems& items, TCalcContext& context) const {
        if (context.DbgLog().IsEnabled()) {
            TStringStream ss;
            for (const auto& item: items) {
                ss << item.ToString() << ' ';
            }
            context.DbgLog() << ss.Str() << '\n';
        }
    }

    static TString AvailCombs2B64(const TShowItems& items) {
        NSc::TValue js;
        for (const auto& item: items) {
            if (!item.IsNoShow()) {
                js.Push(item.ToTValue());
            }
        }
        return Base64Encode(js.ToJson());
    }

    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& context) const override {
        auto showItems = GetEditedShowItemsFromContext(context);
        DbgDumpCombinations(showItems, context);
        if (!showItems) {
            context.DbgLog() << "no avail items to random\n";
            LogValue(context, FEATURE_LOG_PREF + FEAT_SHOW, 0);
            return 0.;
        }
        Sort(showItems);
        TVector<double> weightPartialSums;
        weightPartialSums.reserve(showItems.size());
        double totalWeightSum = showItems[0].Weight;
        weightPartialSums.push_back(totalWeightSum);
        for (auto it = std::next(showItems.begin()); it != showItems.end(); ++it) {
            weightPartialSums.push_back(weightPartialSums.back() + it->Weight);
            totalWeightSum += it->Weight;
        }

        Y_ENSURE(weightPartialSums, "need at least one item");
        Y_ENSURE(weightPartialSums.back() > std::numeric_limits<double>::epsilon(), "too small probabilities, sum is: " + ToString(weightPartialSums.back()));

        const auto& seed = TString{context.GetMeta().RandomSeed().Get()} + AdditionalSeed;
        NExtendedMx::TRandom random(seed);
        const size_t idx = random.Choice(weightPartialSums);

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "seed: " << seed << '\n';
            context.DbgLog() << "partial sums: " << JoinVectorIntoString(weightPartialSums, " ") << '\n';
            context.DbgLog() << "selected idx: " << idx << '\n';
        }

        context.GetResult().FeatureResult()["IsRandom"].Result()->SetBool(true);
        const auto& selectedItem = showItems[idx];
        if (selectedItem.IsNoShow()) {
            context.GetResult().FeatureResult()[FEAT_SHOW].Result()->SetBool(false);
            LogValue(context, FEATURE_LOG_PREF + FEAT_SHOW, 0);
        } else {
            context.GetResult().FeatureResult()[FEAT_SHOW].Result()->SetBool(true);
            context.GetResult().FeatureResult()[FEAT_PLACE].Result()->SetString(selectedItem.Place);
            context.GetResult().FeatureResult()[FEAT_POS].Result()->SetIntNumber(selectedItem.Pos);
            context.GetResult().FeatureResult()[FEAT_VIEWTYPE].Result()->SetString(selectedItem.ViewType);

            LogValue(context, FEATURE_LOG_PREF + FEAT_SHOW, 1);
            LogValue(context, FEATURE_LOG_PREF + FEAT_PLACE, selectedItem.Place);
            LogValue(context, FEATURE_LOG_PREF + FEAT_POS, selectedItem.Pos);
            LogValue(context, FEATURE_LOG_PREF + FEAT_VIEWTYPE, selectedItem.ViewType);
            LogValue(context, FEATURE_AVAIL_PREF + "Comb", AvailCombs2B64(showItems));
        }

        double noShowWeight = MayNotShow ? Scheme().Params().NoShowWeight() / totalWeightSum : 0.0;
        LogValue(context, FEATURE_LOG_PREF + "noshow_p", noShowWeight);
        LogFeatureWeight(*this, context, "Comb", 1 / (selectedItem.Weight / totalWeightSum));
        if (MayNotShow) {
            LogValue(context, "mbskipped", 1);
        }
        return 0.0;
    }

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    const TString AdditionalSeed;
    TWeightedShowItems ShowItemWeights;
    TShowItems RootItems;
    const bool MayNotShow;
};

//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TWinLossBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TWinLossLinearBundle : public TBundleBase<TWinLossLinearConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
    using TInnerFmlParams = TWinLossLinearConstProto::TLinearFmlConst::TParamsConst;
public:
    TWinLossLinearBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        WinCalcer = LoadFormulaProto<TCalcer>(Scheme().Win().Fml(), formulasStoragePtr);
        LossCalcer = LoadFormulaProto<TCalcer>(Scheme().Loss().Fml(), formulasStoragePtr);
        Offset = CalcAdditionalOffset(Scheme().Params().AdditionalFeatures()) + 1;
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "WinLossLinear calculation started\n";

        const size_t stepCount = Scheme().Params().Count();
        TFactors factors;
        FillPredefinedFeatures(this, factors, features, featuresCount, stepCount, Scheme().Params().AdditionalFeatures(), context);

        TVector<double> wins, losses;
        GetLinearFmlResult(Scheme().Win().Params(), factors, *WinCalcer, wins);
        GetLinearFmlResult(Scheme().Loss().Params(), factors, *LossCalcer, losses);

        size_t bestIdx = 0;
        double bestDelta = -std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < wins.size(); ++i) {
            auto d = wins[i] - losses[i];
            if (d > bestDelta && d > 0) {
                bestDelta = d;
                bestIdx = i;
            }
        }
        context.DbgLog() << "selected row: " << bestIdx << '\n';
        ExtractAdditionalFeatsResult(this, Scheme().Params().AdditionalFeatures(), context, factors[bestIdx], true);
        size_t selectedStep = factors[bestIdx][0];
        return TransformStepAndLog(*this, selectedStep, Scheme().Params(), context);
    }

    size_t GetNumFeats() const override {
        return NumFeatsWithOffset(Max(WinCalcer->GetNumFeats(), LossCalcer->GetNumFeats()), Offset);
    }

private:

    void GetLinearFmlResult(const TInnerFmlParams& proto, const TFactors& factors, TCalcer& calcer, TVector<double>& results) const {
        calcer.CalcRelevs(factors, results);
        const auto& a = proto.A();
        const auto& b = proto.B();
        for (auto& r : results) {
            r = a + r * b;
        }
        ApplyResultTransform(results, proto.ResultTransform());
    }

private:
    TCalcer* WinCalcer = nullptr;
    TCalcer* LossCalcer = nullptr;
    size_t Offset = 1;
};

template <typename TParams>
static double SelectAndProcessRowWithFeatsAndPos(const TFactors& factors, const TVector<double>& finalResult, const TExtendedRelevCalcer& calcer,  TCalcContext& context, const TParams& params) {
    const double threshold = params.Threshold();
    const auto idx = params.Greedy()
        ? FirstPositiveIdxElseFirst(finalResult.begin(), finalResult.end(), threshold)
        : MaxIdxIfPositiveElseFirst(finalResult.begin(), finalResult.end(), threshold);
    auto& selectedRow = factors[idx];

    if (context.DbgLog().IsEnabled()) {
        context.DbgLog() << "final result:\n" << JoinVectorIntoString(finalResult, "\t") << '\n';
        context.DbgLog() << "selected row: " << JoinVectorIntoString(selectedRow, "\t") << '\n';
    }
    ExtractAdditionalFeatsResult(&calcer, params.TargetsAdditionalFeatures(), context, selectedRow, true);
    if (const auto& stepName = params.TreatStepAsFeature().Get()) {
        size_t selectedStepOrDefault = finalResult[idx] >= threshold ? selectedRow[0] : params.DefaultStepAsFeature();
        if (params.TreatLastStepAsDontShow() && selectedStepOrDefault == params.Count() - 1) {
            context.DbgLog() << "noshow pos selected" << '\n';
            selectedStepOrDefault = params.DefaultStepAsFeature();
        }

        ProcessStepAsPos(calcer, context, selectedStepOrDefault, stepName);
    }
    return ApplyResultTransform(selectedRow[0], params.ResultFmlTransform());
}

static void LogCombination(const NSc::TValue& combination, const TExtendedRelevCalcer& calcer, TCalcContext& context) {
    for (const auto& k2v : combination.GetDict()) {
        if (k2v.second.IsString() || k2v.second.IsNumber()) {
            LogValue(calcer, context, FEATURE_LOG_PREF + k2v.first, ToString(k2v.second));
            auto res = context.GetResult().FeatureResult()[k2v.first].Result();
            *res.GetRawValue() = k2v.second;
        } else {
            context.DbgLog() << "strange feature: " << k2v.first << " with value: " << k2v.second << '\n';
        }
    }
}

static void SelectCombinationAndLogResult(const TFactors& factors, const TVector<double>& finalResult, const TExtendedRelevCalcer& calcer,  TCalcContext& context,
                                          const NCombinator::TCombinator& combinator, const NCombinator::TCombinationIdxs& combIdxs) {
    const auto idx = MaxIdxIfPositiveElseFirst(finalResult.begin(), finalResult.end());
    if (context.DbgLog().IsEnabled()) {
        auto& selectedRow = factors[idx];
        context.DbgLog() << "final result:\n" << JoinVectorIntoString(finalResult, "\t") << '\n';
        context.DbgLog() << "selected row: " << JoinVectorIntoString(selectedRow, "\t") << '\n';
    }
    const auto prediction = finalResult[idx];
    NSc::TValue combination = prediction >= 0 ? combinator.GetCombinationValues(combIdxs[idx]) : combinator.GetNoShowValues();
    LogCombination(combination, calcer, context);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMultiTargetMx
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiTargetMxBundle : public TBundleBase<TMultiTargetMxConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
    using TCalcers = TVector<TCalcer*>;
public:
    TMultiTargetMxBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        BaseCalcer = Scheme().Params().BaseFml().IsNull() ? nullptr : LoadFormulaProto<TCalcer>(Scheme().Params().BaseFml(), formulasStoragePtr);
        ResultCalcer = LoadFormulaProto<TCalcer>(Scheme().Params().ResultFml(), formulasStoragePtr);

        const auto& params = Scheme().Params();
        const auto& targetFmls = params.Targets();

        size_t targetsCount = targetFmls.Size();
        Y_ENSURE(targetsCount, "not enough target fmls");

        TargetCalcers.reserve(targetFmls.Size());

        const size_t additionalOffset = CalcAdditionalOffset(params.TargetsAdditionalFeatures());
        const size_t additionalOffsetWithStep = additionalOffset + 1;

        size_t baseCalcerFeats = BaseCalcer ? BaseCalcer->GetNumFeats() : 0;
        size_t targetNumFeats = 0;
        for (size_t i = 0; i < +targetsCount; ++i) {
            auto calcer = LoadFormulaProto<TCalcer>(targetFmls[i], formulasStoragePtr);
            targetNumFeats = Max(targetNumFeats, calcer->GetNumFeats());
            TargetCalcers.push_back(calcer);
        }
        targetNumFeats = NumFeatsWithOffset(targetNumFeats, additionalOffsetWithStep);

        ResultBlenderFactorsOffset = params.TargetUseStep() + additionalOffset * params.ResultUseAdditionalFeats();
        ResultNumFeats = params.ResultUseFeats() ? NumFeatsWithOffset(ResultCalcer->GetNumFeats(), ResultBlenderFactorsOffset + TargetCalcers.size()) : 0;
        const size_t resultExpectedFeats = ResultBlenderFactorsOffset + TargetCalcers.size() + ResultNumFeats;
        Y_ENSURE(ResultCalcer->GetNumFeats() <= resultExpectedFeats, "too much feats use in result");

        NumFeats = Max(baseCalcerFeats, targetNumFeats, ResultNumFeats);
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TMultiTargetMxBundle calculation started\n";
        context.DbgLog() << *context.GetMeta().Value__ << "\n";

        const size_t stepCount = Scheme().Params().Count();
        TFactors factors;
        FillPredefinedFeatures(this, factors, features, featuresCount, stepCount, Scheme().Params().TargetsAdditionalFeatures(), context);

        double base = 0.f;
        if (BaseCalcer) {
            base = BaseCalcer->DoCalcRelev(features);
            context.DbgLog() << "base result: " << base << "\n";
            base = ApplyResultTransform(base, Scheme().Params().BaseFmlTransform());
        }
        float from = Scheme().Params().From();
        float step = Scheme().Params().Step();
        for (size_t i = 0; i < factors.size(); ++i) {
            auto& stepF = factors[i][0];
            stepF = base + from + stepF * step;
        }
        DebugFactorsDump("factors", factors, context);

        TVector<TVector<double>> targets;
        CalcMultiple(TargetCalcers, factors, targets);
        DebugFactorsDump("targets res", targets, context);
        TFactors feats4Result;
        Transpose(targets, feats4Result, ResultBlenderFactorsOffset, ResultNumFeats);
        DebugFactorsDump("targets res transposed", feats4Result, context);

        MultiCopyN(factors, feats4Result, ResultBlenderFactorsOffset);
        if (ResultNumFeats) {
            MultiCopyFeatures(feats4Result, features, ResultNumFeats, ResultBlenderFactorsOffset + targets.size());
        }
        DebugFactorsDump("feats 4 result ", feats4Result, context);

        TVector<double> finalResult;
        ResultCalcer->CalcRelevs(feats4Result, finalResult);

        return SelectAndProcessRowWithFeatsAndPos(factors, finalResult, *this, context, Scheme().Params());
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* BaseCalcer = nullptr;
    TCalcer* ResultCalcer = nullptr;
    TCalcers TargetCalcers;
    size_t ResultBlenderFactorsOffset = 0;
    size_t ResultNumFeats = 0;
    size_t NumFeats = std::numeric_limits<size_t>::max();
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMultiTargetMxFastBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiTargetMxFastBundle : public TBundleBase<TMultiTargetMxFastConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
    using TCalcers = TVector<TCalcer*>;
public:
    TMultiTargetMxFastBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        BaseCalcer = Scheme().Params().BaseFml().IsNull() ? nullptr : LoadFormulaProto<TCalcer>(Scheme().Params().BaseFml(), formulasStoragePtr);
        ResultCalcer = LoadFormulaProto<TCalcer>(Scheme().Params().ResultFml(), formulasStoragePtr);

        const auto& params = Scheme().Params();
        const auto& targetFmls = params.Targets();

        size_t targetsCount = targetFmls.Size();
        Y_ENSURE(targetsCount, "not enough target fmls");

        WithCalcers.reserve(targetFmls.Size());
        WithoutCalcers.reserve(targetFmls.Size());

        const size_t additionalOffset = CalcAdditionalOffset(params.TargetsAdditionalFeatures());
        const size_t additionalOffsetWithStep = additionalOffset + 1;

        size_t baseCalcerFeats = BaseCalcer ? BaseCalcer->GetNumFeats() : 0;
        size_t targetNumFeats = 0;
        TargetSize.reserve(targetsCount);
        for (const auto& target : targetFmls) {
            const auto& subLen = target.With().Size();
            Y_ENSURE(subLen, "node should have some subtargets");
            Y_ENSURE(subLen == target.Without().Size(), "with and without fmls should be equal size");
            for (size_t i = 0; i < subLen; ++i) {
                auto withCalcer = LoadFormulaProto<TCalcer>(target.With()[i], formulasStoragePtr);
                auto withoutCalcer = LoadFormulaProto<TCalcer>(target.Without()[i], formulasStoragePtr);
                targetNumFeats = Max(targetNumFeats, withCalcer->GetNumFeats(), withoutCalcer->GetNumFeats());
                WithCalcers.push_back(withCalcer);
                WithoutCalcers.push_back(withoutCalcer);
            }
            TargetSize.push_back(subLen);
        }
        targetNumFeats = NumFeatsWithOffset(targetNumFeats, additionalOffsetWithStep);

        ResultBlenderFactorsOffset = params.TargetUseStep() + additionalOffset * params.ResultUseAdditionalFeats();
        ResultNumFeats = params.ResultUseFeats() ? NumFeatsWithOffset(ResultCalcer->GetNumFeats(), ResultBlenderFactorsOffset + WithCalcers.size()) : 0;
        const size_t resultExpectedFeats = ResultBlenderFactorsOffset + WithCalcers.size() + ResultNumFeats;
        Y_ENSURE(ResultCalcer->GetNumFeats() <= resultExpectedFeats, "too much feats use in result");

        NumFeats = Max(baseCalcerFeats, targetNumFeats, ResultNumFeats);
    }

private:

    void SummarizeTargetsResultByRow(TVector<TVector<double>>& differentTargets) const {
        const size_t totalRows = differentTargets[0].size();
        for (size_t rowIdx = 0; rowIdx < totalRows; ++rowIdx) {
            size_t targetBegin = 0;
            for (const auto& tsz : TargetSize) {
                for (size_t i = 1; i < tsz; ++i) {
                    const auto iOff = i + targetBegin;
                    differentTargets[iOff][rowIdx] += differentTargets[iOff - 1][rowIdx];
                }
                targetBegin += tsz;
            }
        }
    }

    void CalcTargetsTransposed(const TFactors& factors, TVector<TVector<float>>& results, const size_t keepOffset, const size_t tail = 0) const {
        Y_ENSURE(factors && factors[0], "expected not empty factors");
        const size_t totalRows = factors.size();
        TVector<TVector<double>> withRes, withoutRes;
        TVector<const float*> withoutFactors(1, factors[0].data());
        CalcMultiple(WithCalcers, factors, withRes);
        CalcMultiple(WithoutCalcers, withoutFactors, withoutRes);

        results.clear();
        results.reserve(totalRows);
        SummarizeTargetsResultByRow(withoutRes);
        SummarizeTargetsResultByRow(withRes);
        for (size_t rowIdx = 0; rowIdx < totalRows; ++rowIdx) {
            results.emplace_back();
            auto& resRow = results.back();
            resRow.resize(withRes.size() + keepOffset + tail);
            for (size_t targIdx = 0; targIdx < withRes.size(); ++targIdx) {
                resRow[targIdx + keepOffset] = withoutRes[targIdx][0] + withRes[targIdx][rowIdx];
            }
        }
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TMultiTargetMxFastBundle calculation started\n";
        context.DbgLog() << *context.GetMeta().Value__ << "\n";

        const size_t stepCount = Scheme().Params().Count();
        TFactors factors;
        FillPredefinedFeatures(this, factors, features, featuresCount, stepCount, Scheme().Params().TargetsAdditionalFeatures(), context);

        double base = 0.f;
        if (BaseCalcer) {
            base = BaseCalcer->DoCalcRelev(features);
            context.DbgLog() << "base result: " << base << "\n";
            base = ApplyResultTransform(base, Scheme().Params().BaseFmlTransform());
        }
        float from = Scheme().Params().From();
        float step = Scheme().Params().Step();
        for (size_t i = 0; i < factors.size(); ++i) {
            auto& stepF = factors[i][0];
            stepF = base + from + stepF * step;
        }
        DebugFactorsDump("factors", factors, context);

        TFactors feats4Result;
        CalcTargetsTransposed(factors, feats4Result, ResultBlenderFactorsOffset, ResultNumFeats);
        DebugFactorsDump("targets res transposed", feats4Result, context);

        MultiCopyN(factors, feats4Result, ResultBlenderFactorsOffset);
        if (ResultNumFeats) {
            MultiCopyFeatures(feats4Result, features, ResultNumFeats, ResultBlenderFactorsOffset + WithCalcers.size());
        }
        DebugFactorsDump("feats 4 result ", feats4Result, context);

        TVector<double> finalResult;
        ResultCalcer->CalcRelevs(feats4Result, finalResult);

        return SelectAndProcessRowWithFeatsAndPos(factors, finalResult, *this, context, Scheme().Params());
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* BaseCalcer = nullptr;
    TCalcer* ResultCalcer = nullptr;
    TCalcers WithCalcers;
    TCalcers WithoutCalcers;
    TVector<size_t> TargetSize;
    size_t ResultBlenderFactorsOffset = 0;
    size_t ResultNumFeats = 0;
    size_t NumFeats = std::numeric_limits<size_t>::max();
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCascadeMx
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TCascadeMxBundle : public TBundleBase<TCascadeMxConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
    using TCalcers = TVector<TCalcer*>;
public:
    TCascadeMxBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        ResultCalcer = LoadFormulaProto<TCalcer>(Scheme().Params().ResultFml(), formulasStoragePtr);

        const auto& params = Scheme().Params();
        const auto& targetFmls = params.Targets();

        size_t targetsCount = targetFmls.Size();
        Y_ENSURE(targetsCount, "not enough target fmls");

        TargetCalcers.reserve(targetFmls.Size());

        size_t targetNumFeats = 0;
        for (const auto& targetFml: targetFmls) {
            auto calcer = LoadFormulaProto<TCalcer>(targetFml, formulasStoragePtr);
            targetNumFeats = Max(targetNumFeats, calcer->GetNumFeats());
            TargetCalcers.push_back(calcer);
        }

        ResultAdditionalFeats = params.ResultUseFeats() ? NumFeatsWithOffset(ResultCalcer->GetNumFeats(), TargetCalcers.size()) : 0;
        const size_t resultExpectedFeats = TargetCalcers.size() + ResultAdditionalFeats;
        Y_ENSURE(ResultCalcer->GetNumFeats() <= resultExpectedFeats, "result uses too many feats");

        NumFeats = Max(targetNumFeats, ResultAdditionalFeats);
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TCascadeMxBundle calculation started\n";
        context.DbgLog() << *context.GetMeta().Value__ << "\n";

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "factors\tcount: " << featuresCount << '\n';
            context.DbgLog() << JoinStrings(features, features + featuresCount, "\t") << '\n';
        }

        TVector<float> feats4Result(TargetCalcers.size() + ResultAdditionalFeats);
        for (size_t i = 0; i < TargetCalcers.size(); ++i) {
            feats4Result[i] = TargetCalcers[i]->DoCalcRelev(features);
        }

        if (ResultAdditionalFeats) {
            CopyN(features, ResultAdditionalFeats, feats4Result.data() + TargetCalcers.size());
        }

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "feats 4 result\tcount:" << feats4Result.size() << '\n';
            context.DbgLog() << JoinVectorIntoString(feats4Result, "\t") << '\n';
        }

        double resultValue = ResultCalcer->CalcRelev(feats4Result);

        context.DbgLog() << "result value:\t" << resultValue << '\n';

        double finalResult = ApplyResultTransform(resultValue, Scheme().Params().ResultFmlTransform());

        context.DbgLog() << "final result:\t" << finalResult << '\n';

        return finalResult;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* ResultCalcer = nullptr;
    TCalcers TargetCalcers;
    size_t ResultAdditionalFeats = 0;
    size_t NumFeats = std::numeric_limits<size_t>::max();
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TClickIntBruteForce - calcs fml for all availible features combinations
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TClickIntBruteForce : public TBundleBase<TClickIntBruteForceConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;

public:
    TClickIntBruteForce(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Calcer = LoadFormulaProto<TCalcer>(Scheme().Fml(), formulasStoragePtr);
        BaseCalcer = Scheme().HasBaseFml() ? LoadFormulaProto<TCalcer>(Scheme().BaseFml(), formulasStoragePtr) : nullptr;
        HasStep = BaseCalcer;

        size_t fmlFeats = NumFeatsWithOffset(Calcer->GetNumFeats(), CalcAdditionalOffset(Scheme().Params().AdditionalFeatures()) + HasStep);
        NumFeats = Max(fmlFeats, BaseCalcer ? BaseCalcer->GetNumFeats() : 0);
        Y_ENSURE(!Scheme().Params().AdditionalFeatures().Empty(), "need at least one additional feature");
        GetMeta().RegisterAttr(NMeta::FEATURES_INFO);
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "clickintBruteForce calculation started: " << GetAlias() << '\n';
        context.DbgLog() << "input features: " ;
        for (size_t i = 0; i < featuresCount; ++i) {
            context.DbgLog() << *(features + i) << ' ';
        }
        context.DbgLog() << "\n";
        const auto& params = Scheme().Params();

        TFactors factors;
        const size_t stepsCount = HasStep ? 1 : NO_STEP;
        FillPredefinedFeatures(this, factors, features, featuresCount, stepsCount, params.AdditionalFeatures(), context);

        if (BaseCalcer) {
            TVector<float> baseFeats;
            baseFeats.assign(features, features + featuresCount);
            double baseRes = BaseCalcer->CalcRelev(baseFeats);
            Y_ENSURE(factors.size(), "expected to have some factors");
            for (auto& row : factors) {
                row[0] = baseRes;
            }
        }

        DebugFactorsDump("feats", factors, context);
        TVector<double> result;
        Calcer->CalcRelevs(factors, result);

        const auto idx = MaxIdxIfPositiveElseFirst(result.begin(), result.end());
        auto& selectedRow = factors[idx];
        const bool zeroizeIW = result[idx] < 0. && params.ZeroIfBadScore();
        const auto iwRes = zeroizeIW ? 0. : selectedRow[0];

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "final result:\n" << JoinVectorIntoString(result, "\n") << '\n';
            context.DbgLog() << "selected row: " << JoinVectorIntoString(selectedRow, "\t") << '\n';
            context.DbgLog() << "iw res: " << iwRes << '\n';
        }
        ExtractAdditionalFeatsResult(this, params.AdditionalFeatures(), context, selectedRow, HasStep);
        return ApplyResultTransform(iwRes, params.ResultTransform());
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* Calcer = nullptr;
    TCalcer* BaseCalcer = nullptr;
    bool HasStep = false;
    size_t NumFeats = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TPositionalMultiClassSAMBundle - multiclassification bundle
//                                  treat some pos as do not show value
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TPositionalMultiClassSAMBundle : public TBundleBase<TPositionalMultiClassSAMClickIntConstProto> {
    using TRelevCalcer = NMatrixnet::IRelevCalcer;
    using TMnMcCalcer = NMatrixnet::TMnMultiCateg;
public:
    TPositionalMultiClassSAMBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Calcer = LoadFormulaProto<TRelevCalcer, TMnMcCalcer>(Scheme().Fml(), formulasStoragePtr);
        FeatureName = Scheme().Params().FeatureName();
        DefaultPos = Scheme().Params().DefaultPos();

        Y_ENSURE(Calcer, "expected mnmc model");
        NumFeats = Calcer->GetNumFeats();
        Y_ENSURE(FeatureName, "required feature name");
        Y_ENSURE(Calcer->CategValues(), "no categories detected");
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "positional multi class sam calculation started: " << Scheme().Fml().Name() << '\t' << GetAlias() << "\n";
        TFactors factors(1, TVector<float>(features, features + featuresCount));
        DebugFactorsDump("features", factors, context);
        const auto& categValues = Calcer->CategValues();
        TVector<double> result;
        result.resize(categValues.size());
        Calcer->CalcCategs(factors, result.data());
        const auto idxIt = MaxElement(result.begin(), result.end());
        const size_t idx = idxIt - result.begin();

        const double selectedCateg = categValues[idx];
        const double pos = selectedCateg < 0 ? Scheme().Params().DefaultPos() : selectedCateg;

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "categ values:   " << JoinSeq("\t", categValues) << '\n';
            context.DbgLog() << "categ result:   " << JoinSeq("\t", result) << '\n';
            context.DbgLog() << "selected categ: " << selectedCateg << '\n';
            context.DbgLog() << "selected pos: " << pos << '\n';
        }

        ProcessStepAsPos(*this, context, pos, FeatureName);
        return *idxIt;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TMnMcCalcer* Calcer = nullptr;
    TString FeatureName;
    size_t DefaultPos = 0;
    size_t NumFeats = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMultiTargetCombinationsBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiTargetCombinationsBundle : public TBundleBase<TMultiTargetCombinationsConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
    using TCalcers = TVector<TCalcer*>;
public:
    TMultiTargetCombinationsBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , Combinator(*Scheme().Params().Combinations()->GetRawValue()) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        ResultCalcer = LoadFormulaProto<TCalcer>(Scheme().Params().ResultFml(), formulasStoragePtr);

        const auto& params = Scheme().Params();
        const auto& targetFmls = params.Targets();

        size_t targetsCount = targetFmls.Size();
        Y_ENSURE(targetsCount, "not enough target fmls");

        TargetCalcers.reserve(targetFmls.Size());

        const size_t additionalOffset = Combinator.GetOffsetForCombination();

        size_t targetNumFeats = 0;
        for (size_t i = 0; i < +targetsCount; ++i) {
            auto calcer = LoadFormulaProto<TCalcer>(targetFmls[i], formulasStoragePtr);
            targetNumFeats = Max(targetNumFeats, calcer->GetNumFeats());
            TargetCalcers.push_back(calcer);
        }
        targetNumFeats = NumFeatsWithOffset(targetNumFeats, additionalOffset);

        ResultBlenderFactorsOffset = additionalOffset;
        ResultNumFeats = params.ResultUseFeats() ? NumFeatsWithOffset(ResultCalcer->GetNumFeats(), ResultBlenderFactorsOffset + TargetCalcers.size()) : 0;
        const size_t resultExpectedFeats = ResultBlenderFactorsOffset + TargetCalcers.size() + ResultNumFeats;
        Y_ENSURE(ResultCalcer->GetNumFeats() <= resultExpectedFeats, "too much feats use in result");

        NumFeats = Max(targetNumFeats, ResultNumFeats);
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        const auto& contextConst = static_cast<const TCalcContext&>(context);
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TMultiTargetCombinationsBundle calculation started\n";
        context.DbgLog() << *context.GetMeta().Value__ << "\n";

        const auto combIdxs = Combinator.Intersect(contextConst.GetMeta().AvailableVTCombinations());
        if (!combIdxs) {
            context.DbgLog() << "no available combinations\n";
            return 0.;
        }
        TFactors factors = Combinator.FillWithFeatures(combIdxs, features, featuresCount);
        DebugFactorsDump("factors", factors, context);

        TVector<TVector<double>> targets;
        CalcMultiple(TargetCalcers, factors, targets);
        DebugFactorsDump("targets res", targets, context);
        TFactors feats4Result;
        Transpose(targets, feats4Result, ResultBlenderFactorsOffset, ResultNumFeats);
        DebugFactorsDump("targets res transposed", feats4Result, context);

        MultiCopyN(factors, feats4Result, ResultBlenderFactorsOffset);
        if (ResultNumFeats) {
            MultiCopyFeatures(feats4Result, features, ResultNumFeats, ResultBlenderFactorsOffset + targets.size());
        }
        DebugFactorsDump("feats 4 result ", feats4Result, context);

        TVector<double> finalResult;
        ResultCalcer->CalcRelevs(feats4Result, finalResult);

        SelectCombinationAndLogResult(feats4Result, finalResult, *this, context, Combinator, combIdxs);
        return 0.;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* ResultCalcer = nullptr;
    TCalcers TargetCalcers;
    size_t ResultBlenderFactorsOffset = 0;
    size_t ResultNumFeats = 0;
    size_t NumFeats = std::numeric_limits<size_t>::max();
    NCombinator::TCombinator Combinator;
};


TExtendedCalculatorRegistrator<TClickIntBundle> ExtendedClickIntRegistrator("clickint");
TExtendedCalculatorRegistrator<TClickIntRandom> ExtendedClickIntRandomRegistrator("clickint_random");
TExtendedCalculatorRegistrator<TIdxRandomBundle> ExtendedIdxRandomRegistrator("idx_random");
TExtendedCalculatorRegistrator<TMultiFeatureLocalRandomBundle> TMultiFeatureLocalRandomRegistrator("multi_feature_local_random");
TExtendedCalculatorRegistrator<TViewPlacePosRandomBundle> ExtendedViewPlacePosRegistrator("view_place_pos_random");
TExtendedCalculatorRegistrator<TPositionalClickIntBundle> ExtendedPositionalClickIntRegistrator("positional_clickint");
TExtendedCalculatorRegistrator<TWinLossLinearBundle> WinLossLinearBundleRegistrator("winloss_linear");
TExtendedCalculatorRegistrator<TClickIntBruteForce> ExtendedClickIntBruteForceRegistrator("clickint_brute_force");
TExtendedCalculatorRegistrator<TMultiTargetMxBundle> MultiTargetMxBundleRegistrator("multi_target_mx");
TExtendedCalculatorRegistrator<TMultiTargetMxFastBundle> MultiTargetMxFastBundleRegistrator("multi_target_mx_fast");
TExtendedCalculatorRegistrator<TCascadeMxBundle> CascadeMxBundleRegistrator("cascade_mx");
TExtendedCalculatorRegistrator<TPositionalMultiClassSAMBundle> MultiPositionalSAMBundleRegistrator("positional_multi_sam_clickint");
TExtendedCalculatorRegistrator<TMultiTargetCombinationsBundle> MultiTargetCombinationsBundleRegistrator("multi_target_combinations");
TExtendedCalculatorRegistrator<TWeightLocalRandomBundle> ExtendedWeightLocalRandomRegistrator("weight_random");

