#include "bundle.h"
#include "random.h"

#include <kernel/extended_mx_calcer/interface/common.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/builder.h>


using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TMultiShowRandom
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiShowRandomBundle : public TBundleBase<TMultiShowRandomConstProto> {
    using TParams = TMultiShowRandomParamsConstProto;
public:
    TMultiShowRandomBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , AdditionalSeed(Scheme().Params().AdditionalSeed().Get())
      , MayNotShow(Scheme().Params().NoShowWeight() > 0)
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);
        GetMeta().RegisterAttr(NMeta::FEATURES_INFO);
    }

private:
    bool ShowItemAvailableInContext(TCalcContext& ctx, const TParams::TShowConst& show) const {
        TCalcContextMetaConstProto meta = ctx.GetMeta();
        if (!FeatureValueAvailableInContext(meta.FeatureContext(), "Place", show.Place(), ctx.DbgLog())) {
            return false;
        }
        if (show.HasViewType() && !FeatureValueAvailableInContext(meta.FeatureContext(), "ViewType", show.ViewType(), ctx.DbgLog())) {
            return false;
        }
        if (show.HasViewType() && !CombinationAvailableInContext(meta.AvailableVTCombinations(), show.ViewType(), show.Place(), show.Pos(), ctx.DbgLog())) {
            return false;
        }

        return true;
    }

    bool MultiShowAvailableInContext(TCalcContext& ctx, const TParams::TWeightedShowsConst& shows) const {
        for (size_t i = 0; i <  shows.Shows().Size(); ++i) {
            if (!ShowItemAvailableInContext(ctx, shows.Shows()[i])) {
                return false;
            }
        }
        return true;
    }

public:
    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& context) const override {
        const auto& params = Scheme().Params();
        TVector<double> sums;
        TVector<bool> availableMask;
        sums.emplace_back(params.HasNoShowWeight() ? params.NoShowWeight() : 0.0);
        for (const auto& si : params.ShowItems()) {
            bool available = MultiShowAvailableInContext(context, si);
            double newWeight = available ? si.Weight() + sums.back() : sums.back();
            sums.emplace_back(newWeight);
            availableMask.emplace_back(available);
        }
        Y_ENSURE(sums.back() > 0.0, "nothing to choose from");
        const auto& seed = TString{context.GetMeta().RandomSeed().Get()} + AdditionalSeed;
        NExtendedMx::TRandom random(seed);
        const size_t idx = random.Choice(sums);
        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "available mask: ";
            context.DbgLog() << JoinSeq(" ", availableMask) << "\n";
            context.DbgLog() << "partial sums: ";
            context.DbgLog() << JoinSeq(" ", sums) << "\n";
            context.DbgLog() << "selected idx: " << idx << "\n";
        }

        const double selectedWeight = idx > 0 ? (sums[idx] - sums[idx - 1]) : sums[idx];
        const double relInverseWeight = sums.back() / selectedWeight;
        LogValue(context, FEATURE_WEIGHT_PREF + "MultiShow", relInverseWeight);
        LogValue(context, FEATURE_AVAIL_PREF + "MultiShow", JoinSeq("", availableMask));
        LogValue(context, FEATURE_LOG_PREF + "MultiShow", idx);

        if (idx == 0) {
            // no show
        } else {
            auto& dst = context.GetResult().FeatureResult()["MultiShow"].Result()->SetDict();
            const auto& shows = params.ShowItems()[idx - 1].Shows();
            for (const auto& show : shows) {
                auto& placeDict = dst[show.Place()].SetDict();
                const TString featPrefix = FEATURE_LOG_PREF + show.Place()  + "_";
                placeDict["Show"].SetBool(true);
                placeDict["Place"].SetString(show.Place());
                placeDict["Pos"].SetIntNumber(show.Pos());
                LogValue(context, featPrefix + "Show", 1);
                LogValue(context, featPrefix + "Pos", show.Pos());
                if (show.HasViewType()) {
                    placeDict["ViewType"].SetString(show.ViewType());
                    LogValue(context, featPrefix + "ViewType", show.ViewType());
                }
            }
        }

        if (MayNotShow) {
            LogValue(context, "mbskipped", 1);
        }
        return 0.0;
    }

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    const TString AdditionalSeed;
    const bool MayNotShow;
};

TExtendedCalculatorRegistrator<TMultiShowRandomBundle> ExtendedMultiShowRandomRegistrator("multi_show_random");
