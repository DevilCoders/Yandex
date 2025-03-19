#include "bundle.h"

namespace NExtendedMx {

    const TString FEATURE_AVAIL_PREF = "avail_feature_";
    const TString FEATURE_WEIGHT_PREF = "w_feature_";
    const TString FEATURE_LOG_PREF = "feature_";

    const TMeta& GetTargetMeta(NMatrixnet::IRelevCalcer* const) {
        static const TMeta meta = TMeta();
        return meta;
    }

    const TMeta& GetTargetMeta(TExtendedRelevCalcer* const calcer) {
        return calcer->ViewMeta();
    }

    TString GetAliasFromBundle(const TBundleConstProto& bundle) {
        if (!bundle.Alias().IsNull()) {
            return TString{*bundle.Alias()};
        }
        return TString{*bundle.XtdType()};
    }

    static double Divide(const double numerator, const double denominator, const double deflt = 0) {
        return denominator ? numerator / denominator : deflt;
    }

    static double NoDistrib(double ) {
        return 0;
    }

    static double YUniform(double x) {
        return Divide(x, 1 + x);
    }

    static double XUniform(double x) {
        return Divide(x, 1 - x);
    }

    using TDistribPtr = double(*)(double);
    static TDistribPtr GetDistrib(TStringBuf distrType) {
        if (distrType == "y-uniform") {
            return YUniform;
        }
        if (distrType == "x-uniform") {
            return XUniform;
        }
        return NoDistrib;
    }

    static double ApplyDistrib(double x, TStringBuf distrType) {
        return GetDistrib(distrType)(x);
    }

    static double ApplyStumps(double x, const TStumpsConstProto& stumps) {
        double result = stumps.Default();
        for (const auto& stump : stumps.Stumps()) {
            if (x > stump.Threshold()) {
                result = stump.Value();
            }
        }
        return result;
    }

    double ApplyPiecewiseLinearTransform(double x, const TStumpsConstProto& transformParams) {
        const auto& stumps = transformParams.Stumps();
        if (stumps.IsNull() ||  stumps.Empty()) {
            return transformParams.Default();
        }
        size_t rightStumpIdx = 0;
        for (const auto& stump : stumps) {
            if (x > stump.Threshold()) {
                ++rightStumpIdx;
            }
        }
        if (rightStumpIdx == 0) {
            return stumps[0].Value();
        }
        if (rightStumpIdx == stumps.Size()) {
            return stumps[rightStumpIdx - 1].Value();
        }
        const auto& leftStump = stumps[rightStumpIdx -1];
        const auto& rightStump = stumps[rightStumpIdx];
        const double slope = (rightStump.Value() - leftStump.Value()) / (rightStump.Threshold() - leftStump.Threshold());
        return leftStump.Value() + slope * (x - leftStump.Threshold());
    }

    void ApplyResultTransform(TVector<double>& res, const TResultTransformConstProto& rtp) {
        const bool needDistrib = !rtp.Distrib()->empty();
        auto distr = GetDistrib(rtp.Distrib());
        const bool needClamp = !rtp.Clamp().IsNull();
        const double min = needClamp ? rtp.Clamp().Min() : -std::numeric_limits<double>::infinity();
        const double max = needClamp ? rtp.Clamp().Max() : std::numeric_limits<double>::infinity();
        const bool needPiecewiseLinear = !rtp.PiecewiseLinearTransform().IsNull();
        for (auto& r : res) {
            if (needClamp) {
                r = ClampVal(r, min, max);
            }
            if (needDistrib) {
                r = distr(r);
            }
            if (needPiecewiseLinear) {
                r = ApplyPiecewiseLinearTransform(r, rtp.PiecewiseLinearTransform());
            }
        }
    }

    double ApplyResultTransform(double res, const TResultTransformConstProto& rtp) {
        if (!rtp.Clamp().IsNull()) {
            res = ClampVal<double>(res, rtp.Clamp().Min(), rtp.Clamp().Max());
        }
        if (!rtp.Distrib()->empty()) {
            res = ApplyDistrib(res, rtp.Distrib());
        }
        if (!rtp.Stumps()->IsNull()) {
            res = ApplyStumps(res, rtp.Stumps());
        }
        if (!rtp.PiecewiseLinearTransform().IsNull()) {
            res = ApplyPiecewiseLinearTransform(res, rtp.PiecewiseLinearTransform());
        }
        return res;
    }

    size_t NumFeatsWithOffset(const size_t totalFeats, const size_t offset) {
        return totalFeats > offset ? totalFeats - offset : 0;
    }

    size_t StepToPos(float step) {
        return static_cast<size_t>(step + 0.5);
    }

    void ProcessStepAsPos(const TExtendedRelevCalcer& calcer, TCalcContext& context, float step, const TStringBuf& stepName) {
        const size_t pos = StepToPos(step);
        LogValue(calcer, context, FEATURE_LOG_PREF + stepName, pos);
        context.GetResult().FeatureResult()[stepName].Result()->SetIntNumber(pos);
    }

    TVector<const float*> CastTFactorsToConstFloatPtr(const TFactors& features) {
        TVector<const float*> result;
        result.reserve(features.size());
        for (const auto& row : features) {
            result.push_back(row.data());
        }
        return result;
    }

    TVector<TConstArrayRef<float>> CastTFactorsToConstArrayRef(const TFactors& features) {
        TVector<TConstArrayRef<float>> result;
        result.reserve(features.size());
        for (const auto& row : features) {
            result.emplace_back(row.data(), row.size());
        }
        return result;
    }

    TVector<TVector<TStringBuf>> CastTCategoricalFactorsToTStringBufVector(const TCategoricalFactors& catFeatures) {
        TVector<TVector<TStringBuf>> result;
        result.reserve(catFeatures.size());
        for (const auto& row : catFeatures) {
            TVector<TStringBuf> bufRow;
            bufRow.reserve(row.size());
            for (const auto& value : row) {
                bufRow.emplace_back(value);
            }
            result.push_back(bufRow);
        }
        return result;
    }

    void CalcMultiple(
            const TVector<const NCatboostCalcer::TCatboostCalcer*>& calcers,
            const TVector<TConstArrayRef<float>>& fPtrs,
            const TVector<TVector<TStringBuf>>& catFeaturesBufs,
            TVector<TVector<double>>& results) {
        results.resize(calcers.size());
        for (size_t i = 0; i < calcers.size(); ++i) {
            results[i].resize(fPtrs.size());
            calcers[i]->CalcRelevs(fPtrs, catFeaturesBufs, results[i]);
        }
    }
} // NExtendedMx
