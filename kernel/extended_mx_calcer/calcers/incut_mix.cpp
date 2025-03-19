#include "incut_mix.h"
#include "bundle.h"
#include "random.h"

#include <util/generic/ptr.h>
#include <util/generic/yexception.h>


using namespace NExtendedMx;

namespace {

    using TMask = ui32;

    struct TIncut {
        size_t Position;
        size_t Size;
    };

    void SetIncutResult(TCalcContext& ctx, const TIncut& incut, bool refuse, size_t topLength, const TStringBuf& mixType) {
        ctx.GetResult().FeatureResult()[NIncutMixNames::REFUSE_FEATURE].Result()->SetBool(refuse);
        ctx.GetResult().FeatureResult()[NIncutMixNames::POS_FEATURE].Result()->SetIntNumber(incut.Position);
        ctx.GetResult().FeatureResult()[NIncutMixNames::SIZE_FEATURE].Result()->SetIntNumber(incut.Size);
        ctx.GetResult().FeatureResult()[NIncutMixNames::MIX_TYPE_FEATURE].Result()->SetString(mixType);
        ctx.GetResult().FeatureResult()[NIncutMixNames::MIX_TOP_LENGTH_FEATURE].Result()->SetIntNumber(topLength);
    }

    void SetMaskResult(TCalcContext& ctx, TMask mask, bool refuse, size_t topLength, const TStringBuf& mixType) {
        ctx.GetResult().FeatureResult()[NIncutMixNames::REFUSE_FEATURE].Result()->SetBool(refuse);
        ctx.GetResult().FeatureResult()[NIncutMixNames::MASK_FEATURE].Result()->SetIntNumber(mask);
        ctx.GetResult().FeatureResult()[NIncutMixNames::MIX_TYPE_FEATURE].Result()->SetString(mixType);
        ctx.GetResult().FeatureResult()[NIncutMixNames::MIX_TOP_LENGTH_FEATURE].Result()->SetIntNumber(topLength);
    }

    const size_t INCUT_FACTOR_OFFSET = 2; // incut position and size
    const ui32 MAX_INCUT_MIX_TOP_LENGTH = 100; // number of incut size and position combination is O(n^2), where n - max mix top length
    const ui32 MAX_MASK_MIX_TOP_LENGTH = 10; // number of mask variation is O(2^n), where n - max mix top length

    template <typename TInt, typename TFloat>
    TInt Factor2Int(TFloat v) {
        return v > 0 ? static_cast<TInt>(v + 0.5) : static_cast<TInt>(v - 0.5);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TClickIntIncutMixBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TClickIntIncutMixBundle : public TBundleBase<TClickIntIncutMixConstProto> {
    using TMxCalcer = NMatrixnet::IRelevCalcer;
    using TParams = TClickIntIncutMixConstProto::TParamsConst;

public:
    TClickIntIncutMixBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Calcer = LoadFormulaProto<TMxCalcer>(Scheme().Calcer(), formulasStoragePtr);

        Validate();
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "ci_incut_mix bundle calculation started: " << GetAlias()  << '\n';
        const auto& params = Scheme().Params();

        TFactors factors;
        FillFeatures(factors, features, featuresCount, context);
        DebugFactorsDump("ci_incut_mix features", factors, context);

        TVector<double> predictions;
        Calcer->CalcRelevs(factors, predictions);
        double bestPrediction = params.HasRefuseThreshold() ? params.RefuseThreshold() : -Max<double>();
        bool refuse = true;
        TIncut bestIncut{0, 0};
        for (size_t idx = 0; idx < predictions.size(); ++idx) {
            const double prediction = predictions[idx];
            const auto& factorRow = factors[idx];
            if (prediction > bestPrediction) {
                bestPrediction = prediction;
                refuse = false;
                Y_ENSURE(factorRow.size() > 2, "bad factor row length");
                Y_ENSURE(factorRow[0] >= 0. && factorRow[1] >= 0., "negative incut position or size");
                if (params.ReverseResultOrder()) {
                    // inverted order here - size, position, need to invert
                    bestIncut = TIncut{Factor2Int<size_t>(factorRow[1]), Factor2Int<size_t>(factorRow[0])};
                } else {
                    // correct order here - position, size
                    bestIncut = TIncut{Factor2Int<size_t>(factorRow[0]), Factor2Int<size_t>(factorRow[1])};
                }
            }
        }
        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "ci_incut_mix predictions\n" << JoinVectorIntoString(predictions, "\n") << '\n';
            context.DbgLog() << "ci_incut_mix result: pos=" << bestIncut.Position << ", size=" << bestIncut.Size << ", refuse=" << refuse << '\n';
        }
        SetIncutResult(context, bestIncut, refuse, params.MixTopLength(), params.MixType());
        return bestPrediction;
    }

    size_t GetNumFeats() const override {
        const size_t offset = GetTotalFeaturesOffset(Scheme().Params());
        return Calcer->GetNumFeats() > offset ? (Calcer->GetNumFeats() - offset) : 0;
    }

private:
    void FillFeatures(TFactors& dst, const float* features, const size_t featuresCount, TCalcContext& ctx) const {
        const auto& params = Scheme().Params();
        const size_t offset = GetTotalFeaturesOffset(params);
        for (size_t pos = 0; pos <= params.MixTopLength(); ++pos) {
            size_t sz = pos < params.MixTopLength() ? 1 : 0;
            for (; pos + sz <= params.MixTopLength(); ++sz) {
                TIncut incut{pos, sz};
                dst.push_back(TVector<float>());
                auto& f = dst.back();
                f.reserve(offset + featuresCount);
                f.push_back(params.ReverseResultOrder() ? incut.Size : incut.Position);
                f.push_back(params.ReverseResultOrder() ? incut.Position : incut.Size);
                if (params.UseAllDocFeatures()) {
                    FillAllDocFeatures(f, ctx, params);
                } else {
                    FillDocFeaturesByIncut(f, incut, ctx, params);
                }
                Y_ENSURE(f.size() == offset, Sprintf("Bad features count, expect %lu, real %lu", offset, f.size()));
                f.insert(f.end(), features, features + featuresCount);
            }
        }
    }

    static inline double GetValueFromExtraFeatures(TCalcContext& ctx, TStringBuf key, size_t index, double defVal = 0.0) {
        const auto docFeatures = ctx.GetMeta().ExtraFeatures(key);
        return index < docFeatures.Size() ? docFeatures.ByIndex(index)->GetNumber() : defVal;
    }

    static inline size_t GetExtraFeaturesSize(TCalcContext& ctx, TStringBuf key) {
        return ctx.GetMeta().ExtraFeatures(key).Size();
    }

    static void FillAllDocFeatures(TVector<float>& dst, TCalcContext& ctx, const TParams& params) {
        auto fillOneSource = [&params, &ctx, &dst](const auto& keys) {
            for (size_t i = 0; i < params.MixTopLength(); ++i) {
                for (const auto& k : keys) {
                    dst.push_back(GetValueFromExtraFeatures(ctx, k, i));
                }
            }
        };
        fillOneSource(params.IncutDocFeatureKeys());
        fillOneSource(params.OuterDocFeatureKeys());
    }

    static bool IsDocAlreadyAdd(TCalcContext& ctx, TStringBuf oppositePosKey, size_t position, size_t lastOtherPosition) {
        int oppositePosition = Factor2Int<int>(GetValueFromExtraFeatures(ctx, oppositePosKey, position, -1.0));
        return oppositePosition >= 0 && static_cast<size_t>(oppositePosition) < lastOtherPosition;
    }

    static void FillDocFeaturesByIncut(TVector<float>& dst, const TIncut& incut, TCalcContext& ctx, const TParams& params) {
        auto getDocsNm = [&params, &ctx] (TStringBuf key) {
            size_t nm = GetExtraFeaturesSize(ctx, key);
            return nm > 0 ? nm : params.MixTopLength();
        };
        size_t totalIncutDocs = getDocsNm(params.IncutOppositePositionKey());
        size_t totalOuterDocs = getDocsNm(params.OuterOppositePositionKey());

        auto pushFactors = [&ctx, &dst](size_t index, const auto& keys) {
            for (const auto& k : keys) {
                dst.push_back(GetValueFromExtraFeatures(ctx, k, index));
            }
        };
        // incut.Position documents from outer, then incut.Size documents from incut, then others from any available
        size_t totalAdded = 0, incutPos = 0, outerPos = 0;
        for (size_t outerAdded = 0; outerAdded < incut.Position && totalAdded < params.MixTopLength() && outerPos < totalOuterDocs; ++outerPos) {
            if (!IsDocAlreadyAdd(ctx, params.OuterOppositePositionKey(), outerPos, incutPos)) {
                pushFactors(outerPos, params.OuterDocFeatureKeys());
                ++outerAdded;
                ++totalAdded;
            }
        }
        for (size_t incutAdded = 0; incutAdded < incut.Size && totalAdded < params.MixTopLength() && incutPos < totalIncutDocs ; ++incutPos) {
            if (!IsDocAlreadyAdd(ctx, params.IncutOppositePositionKey(), incutPos, outerPos)) {
                pushFactors(incutPos, params.IncutDocFeatureKeys());
                ++incutAdded;
                ++totalAdded;
            }
        }
        for (; totalAdded < params.MixTopLength() && outerPos < totalOuterDocs; ++outerPos) {
            if (!IsDocAlreadyAdd(ctx, params.OuterOppositePositionKey(), outerPos, incutPos)) {
                pushFactors(outerPos, params.OuterDocFeatureKeys());
                ++totalAdded;
            }
        }
        // fill the rest by incut doc features or zeroes
        for (; totalAdded < params.MixTopLength(); ++totalAdded) {
            if (incutPos < totalIncutDocs && !IsDocAlreadyAdd(ctx, params.IncutOppositePositionKey(), incutPos, outerPos)) {
                pushFactors(incutPos++, params.IncutDocFeatureKeys());
            } else {
                pushFactors(outerPos++, params.OuterDocFeatureKeys());
            }
        }
    }

    static size_t GetAllDocFeaturesOffset(const TParams& params) {
        return params.MixTopLength() * (params.IncutDocFeatureKeys().Size() + params.OuterDocFeatureKeys().Size());
    }

    static size_t GetIncutFeaturesOffset(const TParams& params) {
        return params.MixTopLength() * params.IncutDocFeatureKeys().Size(); // assume IncutDocFeatureKeys().Size() == OuterDocFeatureKeys().Size(), see Validate method
    }

    static size_t GetTotalFeaturesOffset(const TParams& params) {
        return INCUT_FACTOR_OFFSET + (params.UseAllDocFeatures() ? GetAllDocFeaturesOffset(params) : GetIncutFeaturesOffset(params));
    }

    void Validate() const {
        const auto& params = Scheme().Params();
        Y_ENSURE(params.IncutDocFeatureKeys().Size() == params.OuterDocFeatureKeys().Size(), "Different doc feature keys size");
        Y_ENSURE(Calcer->GetNumFeats() > GetTotalFeaturesOffset(params), "Not enough features in calcer");
        Y_ENSURE(params.MixTopLength() < MAX_INCUT_MIX_TOP_LENGTH, "Too large MixTopLength");
    }


private:
    TMxCalcer* Calcer = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TRandomIncutMixBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TRandomIncutMixBundle : public TBundleBase<TRandomIncutMixConstProto> {
public:
    TRandomIncutMixBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme)
    {
        Validate();
    }

    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& context) const override {
        context.DbgLog() << "random_incut_mix calculation started: " << GetAlias() << '\n';
        const auto& params = Scheme().Params();
        const TString seed = TString::Join(context.GetMeta().RandomSeed().Get(), params.AdditionalSeed());
        context.DbgLog() << "seed: " << seed << '\n';
        auto incut = GetRandomIncut(seed, params.MixTopLength());
        context.DbgLog() << "random_incut_mix result: pos=" << incut.Position << ", size=" << incut.Size << '\n';
        SetIncutResult(context, incut, false, params.MixTopLength(), params.MixType());
        return 0.0;
    }

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    static TIncut GetRandomIncut(const TString& seed, ui32 topLength) {
        NExtendedMx::TRandom random(seed);
        const ui32 rMax = (topLength + 1) * topLength / 2;
        ui32 rVal = random.NextInt(rMax + 1);
        if (rVal == rMax) {
            return TIncut{topLength, 0};
        }
        ui32 rPos = topLength;
        ui32 incutPosition = 0;
        while (rVal >= rPos) {
            ++incutPosition;
            rVal -= rPos;
            --rPos;
        }
        return TIncut{incutPosition, rVal + 1};
    }

    void Validate() const {
        Y_ENSURE(Scheme().Params().MixTopLength() < MAX_INCUT_MIX_TOP_LENGTH, "Too large MixTopLength");
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TRandomMaskMixBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TRandomMaskMixBundle : public TBundleBase<TRandomMaskMixConstProto> {
public:
    TRandomMaskMixBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme)
    {
        Validate();
    }

    double DoCalcRelevExtended(const float*, const size_t, TCalcContext& context) const override {
        context.DbgLog() << "random_mask_mix calculation started: " << GetAlias() << '\n';
        const auto& params = Scheme().Params();
        const TString seed = TString::Join(context.GetMeta().RandomSeed().Get(), params.AdditionalSeed());
        context.DbgLog() << "seed: " << seed << '\n';
        auto mask = GetRandomMask(seed, params.MixTopLength());
        context.DbgLog() << "random_mask_mix result=" << mask << '\n';
        SetMaskResult(context, mask, false, params.MixTopLength(), params.MixType());
        return 0.0;
    }

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    static TMask GetRandomMask(const TString& seed, ui32 topLength) {
        NExtendedMx::TRandom random(seed);
        static const ui32 maxShift = 31;
        return random.NextInt(1 << Min(maxShift, topLength));
    }

    void Validate() const {
        Y_ENSURE(Scheme().Params().MixTopLength() < MAX_MASK_MIX_TOP_LENGTH, "Too large MixTopLength");
    }
};


TExtendedCalculatorRegistrator<TClickIntIncutMixBundle> ClickIntIncutRegistrator("ci_incut_mix");
TExtendedCalculatorRegistrator<TRandomIncutMixBundle> RandomIncutRegistrator("random_incut_mix");
TExtendedCalculatorRegistrator<TRandomMaskMixBundle> RandomMaskRegistrator("random_mask_mix");
