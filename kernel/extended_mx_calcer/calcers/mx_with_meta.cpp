#include "bundle.h"

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMxWithMetaMxs - calculate metafactors using some mx formulas befaore main mx formula
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMxWithMetaMxsBundle : public TBundleBase<TMxWithMetaMxsConstProto> {
    using TCalcer = NMatrixnet::IRelevCalcer;
    using TCalcers = TVector<TCalcer*>;
public:
    TMxWithMetaMxsBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {
    }

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        ResultCalcer = LoadFormulaProto<TCalcer>(Scheme().UpperFml(), formulasStoragePtr);
        NumFeats = ResultCalcer->GetNumFeats();

        const auto& subFmls = Scheme().SubFmls();

        size_t subFmlsCount = subFmls.Size();

        SubCalcers.reserve(subFmlsCount);
        FactorIndices.reserve(subFmlsCount);
        for (const auto& subFml: subFmls) {
            auto calcer = LoadFormulaProto<TCalcer>(subFml.SubFml(), formulasStoragePtr);
            if (calcer->GetNumFeats() > NumFeats) {
                NumFeats = calcer->GetNumFeats();
            }
            SubCalcers.push_back(calcer);
            FactorIndices.push_back(subFml.FactorIndex());
        }
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TMxWithMetaMxsBundle calculation started\n";
        context.DbgLog() << *context.GetMeta().Value__ << "\n";

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "factors\tcount: " << featuresCount << '\n';
            context.DbgLog() << JoinStrings(features, features + featuresCount, "\t") << '\n';
        }

        TVector<float> modifiedFeatures(ResultCalcer->GetNumFeats());
        CopyN(features, ResultCalcer->GetNumFeats(), modifiedFeatures.data());
        for (size_t i = 0; i < SubCalcers.size(); ++i) {
            modifiedFeatures[FactorIndices[i]]  = SubCalcers[i]->DoCalcRelev(features);
        }

        double resultValue = ResultCalcer->CalcRelev(modifiedFeatures);

        context.DbgLog() << "result value:\t" << resultValue << '\n';

        double finalResult = ApplyResultTransform(resultValue, Scheme().ResultTransform());

        context.DbgLog() << "final result:\t" << finalResult << '\n';
        LogValue(context, "result", finalResult);

        return finalResult;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* ResultCalcer = nullptr;
    TCalcers SubCalcers;
    size_t NumFeats;
    TVector<ui32> FactorIndices;
};

TExtendedCalculatorRegistrator<TMxWithMetaMxsBundle> MxWithMetaMxsBundleRegistrator("mx_with_meta");
