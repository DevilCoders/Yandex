#include "bundle.h"

#include <kernel/ethos/lib/reg_tree/compositions.h>

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TClickRegTreeBundle - simple clickregtree calcer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TClickRegTreeBundle : public TBundleBase<TClickRegTreeConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
public:
    TClickRegTreeBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Calcer = LoadFormulaProto<TCalcer>(Scheme().Fml(), formulasStoragePtr);
    }

private:
    double CalcClickRegTree(NExtendedMx::TCalcContext& context, const float* fPtr, const size_t /* fSize */) const
    {
        const auto& crtParams = Scheme().Params();

        const double step = crtParams.Step();
        const size_t stepsCount = crtParams.StepsCount();
        const double threshold = crtParams.Threshold();
        const TString clickRegTreeType(crtParams.ClickRegTreeType());
        const double startIntentWeight = crtParams.StartIntentWeight().IsNull() ? 0. : crtParams.StartIntentWeight();

        context.DbgLog() << "click reg_tree calculation started: " << Scheme().Fml().Name() << '\t' << GetAlias() << '\n'
                         << "step, stepsCount, threshold, type: " << step << '\t' << stepsCount << '\t' << threshold << '\t' << clickRegTreeType << '\n';

        const bool isPositional = !crtParams.PositionParams().IsNull();

        const double predictedIntentWeight = NRegTree::GetBestIntentWeight(Calcer, fPtr, step, stepsCount, threshold, clickRegTreeType, startIntentWeight, isPositional);
        const double transformedResult = ApplyResultTransform(predictedIntentWeight, crtParams.ResultTransform());

        double actualResult = transformedResult;
        if (isPositional) {
            if (actualResult < 0) {
                actualResult = crtParams.PositionParams().DefaultPos();
            }
            ProcessStepAsPos(*this, context, actualResult, crtParams.PositionParams().FeatureName());
        }
        return actualResult;
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        return CalcClickRegTree(context, features, featuresCount);
    }

    size_t GetNumFeats() const override {
        return NumFeatsWithOffset(Calcer->GetNumFeats(), 1);
    }

private:
    TCalcer* Calcer = nullptr;
};

TExtendedCalculatorRegistrator<TClickRegTreeBundle> ExtendedClickRegTreeRegistrator("clickregtree");
