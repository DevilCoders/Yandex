#include "bundle.h"

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSurplusPredict - calculate one fml, and log another
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TSurplusPredictBundle : public TBundleBase<TSurplusPredictConstProto> {
    using TRealCalcer = TExtendedRelevCalcer;
    using TPredictCalcer = NMatrixnet::IRelevCalcer;
public:
    TSurplusPredictBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        RealCalcer = LoadFormulaProto(Scheme().RealCalcer(), formulasStoragePtr);
        PredictCalcer = LoadFormulaProto<TPredictCalcer>(Scheme().PredictCalcer(), formulasStoragePtr);
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "Surplus control calculation started\n";
        auto res = RealCalcer->DoCalcRelevExtended(features, featuresCount, context);

        TVector<float> feats4ctrl;
        feats4ctrl.reserve(featuresCount + 1);
        feats4ctrl.push_back(res);
        feats4ctrl.insert(feats4ctrl.end(), features, features + featuresCount);
        auto controlRes = PredictCalcer->CalcRelev(feats4ctrl);
        LogValue(context, "control_res", controlRes);
        return res;
    }

    size_t GetNumFeats() const override {
        return RealCalcer->GetNumFeats();
    }

private:
    TRealCalcer* RealCalcer = nullptr;
    TPredictCalcer* PredictCalcer = nullptr;
};


TExtendedCalculatorRegistrator<TSurplusPredictBundle> SurplusPredictRegistrator("surplus_predict");
