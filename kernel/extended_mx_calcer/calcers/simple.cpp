#include "bundle.h"

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSimpleBundle - wrapper for not extended calcers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TSimpleBundle : public TBundleBase<TSimpleConstProto> {
    using TCalcer = TExtendedRelevCalcer;
public:
    TSimpleBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Calcer = LoadFormulaProto<TCalcer>(Scheme().Fml(), formulasStoragePtr);
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "Simple calculation started\n";
        double res = Calcer->DoCalcRelevExtended(features, featuresCount, context);
        res = ApplyResultTransform(res, Scheme().ResultTransform());
        LogValue(context, "result", res);

        auto positionalParams = Scheme().PositionalParams();
        if (!positionalParams.IsNull()) {
            context.GetResult().FeatureResult()[positionalParams.FeatureName()].Result()->SetIntNumber(res);
        }

        return res;
    }

    size_t GetNumFeats() const override {
        return Calcer->GetNumFeats();
    }

private:
    TCalcer* Calcer = nullptr;
};


TExtendedCalculatorRegistrator<TSimpleBundle> SimpleBundleRegistrator("simple");
