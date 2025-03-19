#include "bundle.h"

#include <library/cpp/expression/expression.h>
#include <util/generic/ptr.h>

using namespace NExtendedMx;

/**
 * TMultiBundleByFactors class selects bundle in runtime based on factors values via expression.
 */
class TMultiBundleByFactors : public TBundleBase<TMultiBundleByFactorsConstProto> {

    using TCalcer = TExtendedRelevCalcer;
    using TCalcers = TVector<TCalcer*>;

public:
    TMultiBundleByFactors(const NSc::TValue& scheme)
        : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        const auto& bundles = Scheme().Bundles();
        for (const auto& bundle : bundles) {
            auto calcer = LoadFormulaProto(bundle, formulasStoragePtr);

            if (calcer->GetNumFeats() > NumFeats) {
                NumFeats = calcer->GetNumFeats();
            }

            Calcers.push_back(calcer);
        }
    }

    double DoCalcRelevExtended(const float* features,
                               const size_t featuresCount,
                               TCalcContext& context) const override
    {
        context.DbgLog() << "MultiBundleByFactors calculation started. ";
        THashMap<TString, TString> data;

        // Dump factor values to the expression dictionary.
        for (size_t i = 0; i < featuresCount; ++i) {
            data["factor_" + ToString(i)] = ToString(features[i]);
        }

        // Select apt subbundle.
        size_t formulaIndex = CalcExpression(TString{*Scheme().Params().Expression()}, data);

        double result = 0;
        if (formulaIndex < Calcers.size()) {
            result = Calcers[formulaIndex]->DoCalcRelevExtended(features, featuresCount, context);
        }
        else {
            context.DbgLog() << "Unexpected result of bundle index expression. "
                             << "Bundles size is: " << Calcers.size() << " "
                             << "Expression result is: " << formulaIndex << " ";
        }
        return result;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcers Calcers;
    size_t NumFeats = 0;
};

TExtendedCalculatorRegistrator<TMultiBundleByFactors> ExtendedMultiBundleByFactorRegistrator("multibundle_by_factors");
