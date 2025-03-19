#include "bundle.h"

#include <library/cpp/expression/expression.h>

#include <util/generic/ptr.h>

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMultiBundle - calc multiple bundles with expressions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiBundle : public TBundleBase<TMultiBundleConstProto> {
    using TCalcer = TExtendedRelevCalcer;
    using TCalcers = TVector<TCalcer*>;
public:
    TMultiBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        const auto& bundles = Scheme().Bundles();
        Calcers.reserve(bundles.Size());
        for (const auto& bundle : bundles) {
            auto calcer = LoadFormulaProto(bundle, formulasStoragePtr);
            if (calcer->GetNumFeats() > NumFeats) {
                NumFeats = calcer->GetNumFeats();
            }
            Calcers.push_back(calcer);
        }
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "MultiBundle calculation started\n";
        THashMap<TString, TString> data;
        bool logChildRes = Scheme().Params().LogChildResult();
        for (size_t i = 0; i < Calcers.size(); ++i) {
            auto res = Calcers[i]->DoCalcRelevExtended(features, featuresCount, context);
            const auto childAlias = Calcers[i]->GetAlias();
            data["fml." + childAlias] = ToString(res);
            if (logChildRes) {
                LogValue(context, "child_" + childAlias, res);
            }
        }
        double res = CalcExpression(TString{*Scheme().Params().Expression()}, data);
        res = ApplyResultTransform(res, Scheme().Params().ResultTransform());
        LogValue(context, "result", res);
        return res;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcers Calcers;
    size_t NumFeats = 0;
};

TExtendedCalculatorRegistrator<TMultiBundle> ExtendedMultiBundleRegistrator("multibundle");
