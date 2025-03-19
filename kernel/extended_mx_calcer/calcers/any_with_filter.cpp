#include <kernel/extended_mx_calcer/interface/common.h>

#include "bundle.h"
#include <iostream>

using namespace NExtendedMx;

class TAnyWithFilter : public TBundleBase<TAnyWithFilterConstProto> {
    using TRecalcParamsConstProto = TAnyWithFilterConstProto::TRecalcParamsConst;
public:

    TAnyWithFilter(const NSc::TValue& scheme)
            : TBundleBase(scheme) {}
    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        ProdBundle = LoadFormulaProto(Scheme().ProdBundle(), formulasStoragePtr);
        FilterModel = LoadFormulaProto<NMatrixnet::IRelevCalcer>(Scheme().FilterModel(), formulasStoragePtr);
        Y_ENSURE(ProdBundle && FilterModel);
        NumFeats = Max(ProdBundle->GetNumFeats(), FilterModel->GetNumFeats());
    }

    double DoFinishWork(const double res, TCalcContext& context, const double filterRes, const double filterThreshold, const auto& params) const {
        LogValue(context, "filter_threshold", filterThreshold);
        bool filterVerdict = ((filterRes > filterThreshold) == params.FilterIfLess());
        LogValue(context, "filter_verdict", filterVerdict);

        if (!filterVerdict) {
            context.DbgLog() << "Filter triggered, return default value = " << params.DefaultValue() <<
                    ", setting default pos = " << params.DefaultPos() << "\n";
            LogValue(context, FEATURE_LOG_PREF + FEAT_POS, params.DefaultPos());
            LogValue(context, "pos_before_filter",
                context.GetResult().FeatureResult()["Pos"].Result()->GetIntNumber());
            context.GetResult().FeatureResult()["Pos"].Result()->SetIntNumber(params.DefaultPos());
            context.GetResult().IsFiltered() = true;
            return params.DefaultValue();
        }
        context.DbgLog() << "Return ordinal result = " << res << "\n";
        context.GetResult().RawPredictions().Aux()["filter_result"] = filterRes;
        return res;
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TAnyWithFilter calculation started\n";
        const auto& params = Scheme().Params();
        const double filterRes = FilterModel->DoCalcRelev(features);
        context.DbgLog() << "filter value = " << filterRes << "\n";
        LogValue(context, "filter_value", filterRes);
        auto res = ProdBundle->DoCalcRelevExtended(features, featuresCount, context);
        return DoFinishWork(res, context, filterRes, params.Threshold(), params);
    }

    double RecalcRelevExtended(TCalcContext& context, const NSc::TValue& recalcParams) override {
        context.DbgLog() << "TAnyWithFilter recalculation started\n";
        const auto& params = Scheme().Params();
        TRecalcParamsConstProto recalcParamsProto(&recalcParams);
        const double filterRes = context.GetResult().RawPredictions().Aux()["filter_result"]->GetNumber();
        context.DbgLog() << "filter value = " << filterRes << "\n";
        LogValue(context, "filter_value", filterRes);
        auto res = ProdBundle->RecalcRelevExtended(context, *recalcParamsProto.ProdBundleParams().GetRawValue());
        return DoFinishWork(
            res,
            context,
            filterRes,
            recalcParamsProto.Params().HasThreshold() ? recalcParamsProto.Params().Threshold() : params.Threshold(),
            params
        );
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TExtendedRelevCalcer* ProdBundle = nullptr;
    NMatrixnet::IRelevCalcer* FilterModel = nullptr;
    size_t NumFeats = 0;
};

TExtendedCalculatorRegistrator<TAnyWithFilter> ExtendedAnyWithFilterRegistrator("any_with_filter");
