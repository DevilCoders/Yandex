#include "bundle.h"
#include "clickint.h"
#include "random.h"

#include <library/cpp/perceptron/perceptron.h>

#include <util/generic/ymath.h>
#include <library/cpp/string_utils/base64/base64.h>


using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TPositionalPerceptronBundle - calculate subtargets as .info formulas, then pass results to neural network
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TPositionalPerceptronBundle : public TBundleBase<TPositionalPerceptronConstProto> {
    using TRelevCalcer = NMatrixnet::IRelevCalcer;
    using TRelevCalcers = TVector<TRelevCalcer*>;
    using TPerceptronPtr = THolder<NNeuralNetwork::TRumelhartPerceptron>;

public:
    TPositionalPerceptronBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        FeatureName = Scheme().Params().FeatureName();
        Perceptron = MakeHolder<NNeuralNetwork::TRumelhartPerceptron>();
        NumFeats = 0;

        Y_ENSURE(FeatureName, "required feature name");

        const auto& targetFmls = Scheme().Subtargets();
        const size_t targetsCount = targetFmls.Size();
        Subtargets.reserve(targetsCount);
        for (const auto& target : targetFmls) {
            auto sub = LoadFormulaProto<TRelevCalcer>(target, formulasStoragePtr);
            NumFeats = Max(NumFeats, sub->GetNumFeats());
            Subtargets.push_back(sub);
        }
        Positions.reserve(Scheme().Params().Positions().Size());
        for (const auto &pos : Scheme().Params().Positions())
            Positions.push_back(pos);
        Y_ENSURE(Positions.size() > 1, "should be more then one position");

        bool loaded = Perceptron->LoadFromString(Base64Decode(Scheme().Perceptron()));
        Y_ENSURE(loaded, "could not load perceptron");
    }

    void CalcSubtargets(const float *features, const size_t featuresCount, TCalcContext& context, TVector<TVector<double>> &result) const {
        TFactors factors(1, TVector<float>(features, features + featuresCount));
        DebugFactorsDump("features", factors, context);
        CalcMultiple(Subtargets, factors, result);
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "positional perceptron calculation started: " << GetAlias() << "\n";
        TVector<TVector<double>> subResults;
        CalcSubtargets(features, featuresCount, context, subResults);
        TVector<double> factors, scores;
        factors.reserve(subResults.size());
        for (const auto& res : subResults)
            factors.push_back(res.front());
        Perceptron->Calculate(factors, scores);
        size_t res = 0;
        for (size_t i = scores.size() - 1; i > 0; --i) {
            if (scores[i] > scores[res])
                res = i;
        }

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "scores:        " << JoinSeq("\t", scores) << '\n';
            context.DbgLog() << "positions:     " << JoinSeq("\t", Positions) << '\n';
            context.DbgLog() << "selected item: " << res << '\n';
            context.DbgLog() << "selected pos:  " << Positions[res] << '\n';
        }
        ProcessStepAsPos(*this, context, Positions[res], FeatureName);
        return Positions[res];
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TRelevCalcers Subtargets;
    TString FeatureName;
    TVector<i32> Positions;
    TPerceptronPtr Perceptron;
    size_t NumFeats = 0;
};


TExtendedCalculatorRegistrator<TPositionalPerceptronBundle> PositionalPerceptronBundleRegistrator("positional_perceptron");

