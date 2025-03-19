#include "bundle.h"

using namespace NExtendedMx;

/**
 * TMultiBundleWithExternalViewTypeMx class calculates best view type (VT) by external classifier
 * and filter available VT to apply existed formula (with single selected VT).
 *
 * TODO: support possibility of different factor count and content of bundle and multiclassifier
 * (in case of success).
 */
class TMultiBundleWithExternalViewTypeMx :
        public TBundleBase<TMultiBundleWithExternalViewTypeMxConstProto> {

    using TCalcer = TExtendedRelevCalcer;
    using TRelevCalcer = NMatrixnet::IRelevCalcer;
    using TMnMcCalcer = NMatrixnet::TMnMultiCateg;
    using TCategoryProbabilities = TMultiMap<float, TString, TGreater<float>>;

public:
    TMultiBundleWithExternalViewTypeMx(const NSc::TValue& scheme)
        : TBundleBase(scheme) {
    }

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const storagePtr) override
    {
        ExternalMx = LoadFormulaProto<TRelevCalcer, TMnMcCalcer>(Scheme().ExternalMx(), storagePtr);
        RegularBundle = LoadFormulaProto(Scheme().RegularBundle(), storagePtr);

        Names.clear();
        for (size_t i = 0; i < Scheme().ExternalMxClasses().Size(); ++i)
            Names.push_back(Scheme().ExternalMxClasses()[i].Get().data());

        Y_ENSURE(RegularBundle->GetNumFeats(), ExternalMx->GetNumFeats());
    }

    /**
     *  Description:
     *  - Call external multiclassifier, calculate type probabilities.
     *  - Match probabilities with names from learn, sort values.
     *  - Select class with best probability from available viewtypes.
     *  - Call regular bundle with single available vietype.
     *
     *  Subbundle returns result not via result value, but via json in argument reference:
     *
     *  "Log":{"multi_target_mx":{"avail_feature_ViewType":"market_category_vendor",
     *                            "feature_Pos":0,"feature_ViewType":"market_category_vendor"}}
     *
     *  "Result":{"FeatureResult":{"Pos":{"Result":0},"ViewType":{"Result":"market_category_vendor"}}}
     */
    double DoCalcRelevExtended(const float* features,
                               const size_t featuresCount,
                               TCalcContext& context) const override
    {
        context.DbgLog() << "TMultiBundleWithExternalViewTypeMx calculation started.";

        // Prepare factors and get multiclassifier results.
        const auto categories = ExternalMx->CategValues();
        TFactors factors(1, TVector<float>(features, features + featuresCount));
        TVector<double> mnMxResult(categories.size());
        ExternalMx->CalcCategs(factors, mnMxResult.data());

        // Bind names of learned classes from xtd and probabilities.
        TCategoryProbabilities probabilities = CategoryProbabilities(Names, categories, mnMxResult);

        Cdbg << "ExternalMx classifier probabilities:" << Endl;
        context.DbgLog() << "ExternalMx classifier probabilities:\n";
        for (const auto& pair : probabilities) {
            Cdbg << pair.first << ":" << pair.second << Endl;
            context.DbgLog() << pair.first << ":" << pair.second << "\n";
        }

        if (NSc::TValue* meta = context.GetMeta()->GetRawValue()) {
            NSc::TValue& availables = (*meta)["FeatureContext"]["ViewType"]["AvailibleValues"];

            Cdbg << "Availible viewTypes:" << meta->ToJson() << Endl;
            context.DbgLog() << "Availible viewTypes: " << meta->ToJson() << "\n";

            // Select available viewType with maximum probability and remove others.
            for (const auto& pair : probabilities) {
                // Current format of ["ViewType"]["AvailibleValues"] is dictionary with "1" as value.
                // E.g.: {"market_category_vendor":"1","market_implicit_model":"1"}
                if (availables.Get(pair.second).GetString() == "1") {
                    availables.ClearDict();
                    availables[pair.second] = "1";
                    break;
                }
            }

            context.DbgLog() << "Availible viewTypes after fix: " << availables.ToJson() << "\n";
            Cdbg << "Availible viewTypes after fix: " << meta->ToJson() << Endl;
        }
        return RegularBundle->DoCalcRelevExtended(features, featuresCount, context);
    }

    size_t GetNumFeats() const override {
        return RegularBundle->GetNumFeats();
    }
private:

    /**
     *  Build map of class names, ordered by probability.
     *
     *  @names - class (viewType) names, ordered as in learn pool.
     *  @indexes - class indexes in concrete multiclassifier.
     *  @probabilities - class probabilities.
     *
     *  i=2, indexes[i]=5, probabilities[i]=0.12,
     *  that is there is result of class 5 from learn on the position 2, => names[5] <==> probability[2]
     */
    TCategoryProbabilities CategoryProbabilities(const TVector<TString>& names,
                                                 TConstArrayRef<double> indexes,
                                                 const TVector<double>& probabilities) const
    {
        Y_ENSURE(names.size() == indexes.size(),
                 "Unexpected ExternalMxClasses size (or real multiclassifier classes count).");

        Y_ENSURE(indexes.size() == probabilities.size(),
                 "Not equal sizes of categories and probabilities");

        TCategoryProbabilities probabilityToType;
        for (size_t i = 0; i < indexes.size(); ++i)
            probabilityToType.insert({ probabilities[i], names[indexes[i]] });

        return probabilityToType;
    }

private:
    TCalcer* RegularBundle = nullptr;
    TMnMcCalcer* ExternalMx = nullptr;
    TVector<TString> Names;
};

TExtendedCalculatorRegistrator<TMultiBundleWithExternalViewTypeMx>
ExtendedMultiBundleWithExternalViewTypeMxRegistrator("multibundle_with_external_vt_mx");
