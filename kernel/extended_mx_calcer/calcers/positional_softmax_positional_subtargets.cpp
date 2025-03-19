#include "bundle.h"
#include "clickint.h"
#include "random.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/perceptron/perceptron.h>

#include <util/generic/ymath.h>

using namespace NExtendedMx;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TPositionalSoftmaxWithPositionalSubtargetsBundle - multiclassification bundle with subtargets
//                                                    each subtarget has first K binary features - indicators of positions
//                                                    treat some pos as do not show value
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TPositionalSoftmaxWithPositionalSubtargetsBundle : public TBundleBase<TPositionalSoftmaxWithPositionalSubtargetsConstProto> {
    using TRelevCalcer = NMatrixnet::IRelevCalcer;
    using TMnMcCalcer = NMatrixnet::TMnMultiCateg;
    using TRelevCalcers = TVector<TRelevCalcer*>;

public:
    TPositionalSoftmaxWithPositionalSubtargetsBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
       Upper = LoadFormulaProto<TRelevCalcer, TMnMcCalcer>(Scheme().Upper(), formulasStoragePtr);
       FeatureName = Scheme().Params().FeatureName();
       DefaultPos = Scheme().Params().DefaultPos();
       PositionCount = Scheme().Params().PositionCount();
       NumFeats = 0;
       UseSourceFeaturesInUpperFml = Scheme().Params().UseSourceFeaturesInUpperFml();

        Y_ENSURE(Upper, "expected mnmc model");
        size_t upperNumFeats = Upper->GetNumFeats();
        Y_ENSURE(FeatureName, "required feature name");
        Y_ENSURE(Upper->CategValues(), "no categories detected");

        const auto& targetFmls = Scheme().Subtargets();

        const size_t targetsCount = targetFmls.Size();
        Y_ENSURE(targetsCount * PositionCount == upperNumFeats, "bad number of target fmls");

        Subtargets.reserve(targetsCount);
        for (const auto& target : targetFmls) {
            auto sub = LoadFormulaProto<TRelevCalcer>(target, formulasStoragePtr);
            NumFeats = Max(NumFeats, sub->GetNumFeats());
            Subtargets.push_back(sub);
        }
        Y_ENSURE(NumFeats > PositionCount, "bad subtargets");
        NumFeats -= PositionCount;
    }

    void CalcSubtargets(const float *features, const size_t featuresCount, TCalcContext& context, TVector<TVector<double>> &result) const {
        TVector<float> subFeatures(PositionCount);
        subFeatures.insert(subFeatures.end(), features, features + featuresCount);
        TFactors factors(PositionCount, subFeatures);
        for (size_t i = 0; i < PositionCount; ++i)
            factors[i][i] = 1.0;
        DebugFactorsDump("features", factors, context);
        CalcMultiple(Subtargets, factors, result);
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "positional softmax with subtargets calculation started: " << Scheme().Upper().Name() << '\t' << GetAlias() << "\n";
        TVector<TVector<double>> subResults;
        CalcSubtargets(features, featuresCount, context, subResults);
        TFactors factors(1);
        factors[0].reserve(subResults.size() * PositionCount + UseSourceFeaturesInUpperFml * featuresCount);
        for (const auto& res : subResults)
            factors[0].insert(factors[0].end(), res.begin(), res.end());
        if (UseSourceFeaturesInUpperFml)
            factors[0].insert(factors[0].end(), features, features + featuresCount);
        const auto& categValues = Upper->CategValues();
        TVector<double> result(categValues.size());
        Upper->CalcCategs(factors, result.data());
        const auto idxIt = MaxElement(result.begin(), result.end());
        const size_t idx = idxIt - result.begin();

        const double selectedCateg = categValues[idx];
        const double pos = selectedCateg < 0 ? DefaultPos : selectedCateg;

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
    TMnMcCalcer* Upper = nullptr;
    TRelevCalcers Subtargets;
    TString FeatureName;
    size_t DefaultPos = 0;
    size_t PositionCount = 0;
    size_t NumFeats = 0;
    bool UseSourceFeaturesInUpperFml = false;
};


TExtendedCalculatorRegistrator<TPositionalSoftmaxWithPositionalSubtargetsBundle> PositionalSoftmaxWithPositionalSubtargetsBundleRegistrator("positional_softmax_with_positional_subtargets");

