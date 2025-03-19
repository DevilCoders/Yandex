#include "bundle.h"

#include <util/generic/ptr.h>
#include <util/string/cast.h>

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TBinaryBundleBase - base class for calculation model and compare it with threshold
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TSchemeProto>
class TBinaryBundleBase : public TBundleBase<TSchemeProto> {
    using TParent = TBundleBase<TSchemeProto>;
public:
    TBinaryBundleBase(const NSc::TValue& scheme)
        : TParent(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
         BinaryClassifier = TParent::LoadFormulaProto(TParent::Scheme().BinaryClassifier(), formulasStoragePtr);
    }

    virtual ~TBinaryBundleBase() {}

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const final {
        TParent::EnsureFeatureCount(featuresCount);
        DoOnStart(context);
        const auto& params = TParent::Scheme().Params();
        double score = BinaryClassifier->DoCalcRelevExtended(features, featuresCount, context);
        if (score > params.Threshold()) {
            DoOnPositive(context);
            return score;
        }
        DoOnNegative(context);
        return 0.f;
    }

    size_t GetNumFeats() const final {
        return BinaryClassifier->GetNumFeats();
    }

protected:
    virtual void DoOnPositive(TCalcContext& context) const = 0;
    virtual void DoOnNegative(TCalcContext& context) const = 0;
    virtual void DoOnStart(TCalcContext&) const {}

private:
    TExtendedRelevCalcer* BinaryClassifier = nullptr;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TOnePositionBinaryBundle - set position and extra features to calc context on positive
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TOnePositionBinaryBundle : public TBinaryBundleBase<TOnePositionBinaryConstProto> {
public:
    TOnePositionBinaryBundle(const NSc::TValue& scheme)
        : TBinaryBundleBase(scheme)
    {}

    void DoOnStart(TCalcContext& context) const override {
        context.DbgLog() << "OnePositionBinary calculation started\n";
    }

    void DoOnPositive(TCalcContext& context) const override {
        SetPosition(context, Scheme().Params().Position());
        SetAdditionalFeatures(context);
    }

    void DoOnNegative(TCalcContext& context) const override {
        if (Scheme().Params().HasNotShownPosition()) {
            SetPosition(context, Scheme().Params().NotShownPosition());
        }
    }

private:
    void SetAdditionalFeatures(TCalcContext& context) const {
        for (const auto& af : Scheme().Params().AdditionalFeaturesToSet()) {
            context.GetResult().FeatureResult()[af.Key()].Result()->SetString(af.Value());
            LogValue(context, FEATURE_LOG_PREF + ToString(af.Key()), af.Value());
        }
    }

    void SetPosition(TCalcContext& context, ui32 position) const {
        const auto& featureName = Scheme().Params().PositionFeatureName();
        context.GetResult().FeatureResult()[featureName].Result()->SetIntNumber(position);
        LogValue(context, FEATURE_LOG_PREF + ToString(featureName), position);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TBinaryWithArbitraryResultBundle - set arbitrary features to calc context on positive or/and negative
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TBinaryWithArbitraryResultBundle : public TBinaryBundleBase<TBinaryWithArbitraryResultConstProto> {
public:
    TBinaryWithArbitraryResultBundle(const NSc::TValue& scheme)
        : TBinaryBundleBase(scheme)
    {}

    void DoOnPositive(TCalcContext& context) const override {
        SetFeatures(context, Scheme().Params().FeaturesToSetOnPositive());
    }

    void DoOnNegative(TCalcContext& context) const override {
        SetFeatures(context, Scheme().Params().FeaturesToSetOnNegative());
    }

private:
    template <typename TFeatures>
    void SetFeatures(TCalcContext& context, const TFeatures& features) const {
        for (const auto& f : features) {
            context.GetResult().FeatureResult()[f.Key()].Result().Assign(f.Value());
            LogValue(context, FEATURE_LOG_PREF + ToString(f.Key()), *f.Value().GetRawValue());
        }
    }
};

TExtendedCalculatorRegistrator<TBinaryWithArbitraryResultBundle> BinaryWithArbitraryResultRegistrator("binary_with_arbitary_result");
TExtendedCalculatorRegistrator<TOnePositionBinaryBundle> OnePositionBinaryRegistrator("one_position_binary");
