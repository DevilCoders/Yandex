#include "bundle.h"
#include "random.h"

#include <kernel/extended_mx_calcer/interface/common.h>

#include <util/string/builder.h>

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TAnyWithRandom - used for integration randomization in one bundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TAnyWithRandom : public TBundleBase<TAnyWithRandomConstProto> {
    using TCalcer = TExtendedRelevCalcer;
    using TRecalcParamsConstProto = TAnyWithRandomConstProto::TRecalcParamsConst;
public:
    TAnyWithRandom(const NSc::TValue& scheme)
        : TBundleBase(scheme)
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);
    }

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        ProdBundle = LoadFormulaProto(Scheme().ProdBundle(), formulasStoragePtr);
        RandomizedBundle = LoadFormulaProto(Scheme().RandomizeBundle(), formulasStoragePtr);
        Y_ENSURE(ProdBundle && RandomizedBundle);
        NumFeats = Max(ProdBundle->GetNumFeats(), RandomizedBundle->GetNumFeats());
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TAnyWithRandom calculation started\n";
        const double res = ProdBundle->DoCalcRelevExtended(features, featuresCount, context);

        return DoFinishWork(res, context, [&context, &features, featuresCount, this](){return RandomizedBundle->DoCalcRelevExtended(features, featuresCount, context);});
    }

    double RecalcRelevExtended(TCalcContext& context, const NSc::TValue& recalcParams) override {
        context.DbgLog() << "TAnyWithRandom recalculation started\n";

        auto res = ProdBundle->RecalcRelevExtended(context, *TRecalcParamsConstProto(&recalcParams).ProdBundleParams().GetRawValue());

        return DoFinishWork(res, context, [&context, &recalcParams, this](){return RandomizedBundle->RecalcRelevExtended(context, recalcParams);});
    }

    double DoFinishWork(double res, TCalcContext& context, const auto randomCalcFunc) const {
        const auto& params = Scheme().Params();
        const auto seed = TStringBuilder() << context.GetMeta().RandomSeed().Get() << "_" << params.AdditionalSeed();
        NExtendedMx::TRandom random(seed);
        const auto randVal = random.NextReal(1.);
        context.DbgLog() << "random seed = " << seed << ", value = " << randVal << '\n';
        if (randVal < params.RandomProbability()) {
            context.DbgLog() << "apply random bundle\n";
            const auto pos = context.GetResult().FeatureResult()["Pos"].Result();
            if (pos.GetRawValue()->IsIntNumber()) {
                context.GetMeta().PredictedPosInfo() = pos.GetRawValue()->GetIntNumber();
            }
            // do deep copy
            context.GetMeta().PredictedFeaturesInfo().GetRawValue()->CopyFrom(*context.GetResult().GetRawValue());
            context.GetResult().Clear();
            res = randomCalcFunc();
            context.GetResult().FeatureResult()[FEAT_IS_RANDOM].Result()->SetBool(true);
        }
        return res;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TCalcer* ProdBundle = nullptr;
    TCalcer* RandomizedBundle = nullptr;
    size_t NumFeats = 0;
};

TExtendedCalculatorRegistrator<TAnyWithRandom> ExtendedAnyWithRandomRegistrator("any_with_random");
