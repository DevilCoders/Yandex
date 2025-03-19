#include <kernel/extended_mx_calcer/interface/common.h>

#include "bundle.h"
#include <iostream>

using namespace NExtendedMx;

class TTwoCalcersUnion : public TBundleBase<TTwoCalcersUnionConstProto> {
public:
    TTwoCalcersUnion(const NSc::TValue& scheme)
            : TBundleBase(scheme) {}
    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        ProdBundle = LoadFormulaProto(Scheme().ProdBundle(), formulasStoragePtr);
        AdditionalBundle = LoadFormulaProto(Scheme().AdditionalBundle(), formulasStoragePtr);
        Y_ENSURE(ProdBundle && AdditionalBundle);
        NumFeats = Max(ProdBundle->GetNumFeats(), AdditionalBundle->GetNumFeats());
        Priority = Scheme().Priority();
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TwoCalcersUnion calculation started\n";
        context.DbgLog() << Priority << "\n";

        double res;
        if (Priority == "prod") {
            AdditionalBundle->DoCalcRelevExtended(features, featuresCount, context);
            res = ProdBundle->DoCalcRelevExtended(features, featuresCount, context);
        } else {
            res = ProdBundle->DoCalcRelevExtended(features, featuresCount, context);
            AdditionalBundle->DoCalcRelevExtended(features, featuresCount, context);
        }
        return res;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TExtendedRelevCalcer* ProdBundle = nullptr;
    TExtendedRelevCalcer* AdditionalBundle = nullptr;
    size_t NumFeats = 0;
    TString Priority;
};

TExtendedCalculatorRegistrator<TTwoCalcersUnion> ExtendedAnyWithViewTypeRegistrator("two_calcers_union");
