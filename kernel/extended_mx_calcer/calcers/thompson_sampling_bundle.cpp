#include "bundle.h"
#include "clickint.h"
#include "combinations.h"
#include "random.h"
#include "thompson_sampling_distributions.h"
#include "thompson_sampling_bundle.h"

#include <kernel/extended_mx_calcer/interface/common.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <util/digest/murmur.h>
#include <util/generic/ymath.h>
#include <util/random/fast.h>
#include <util/string/builder.h>
#include <util/string/cast.h>


using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TThompsonSampling - generate positions via Thompson Sampling.
// Positional win and loss rt counters are used as params of beta distribution.
// We sample from such distribitions for each postion and choose one with the highest score.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TThompsonSamplingBundle : public TBundleBase<TThompsonSamplingConstProto> {
public:
    TThompsonSamplingBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme)
      , ShowScoreThreshold(Scheme().Params().ShowScoreThreshold())
      , DistributionName(Scheme().Params().DistributionName())
      , Distribution(FromString<EDistribution>(Scheme().Params().DistributionName()))
      , CountersMode(FromString<ECountersMode>(Scheme().Params().CountersMode()))
      , UniformProba(Scheme().Params().UniformProba())
      , DoNotShowPos(Scheme().Params().DoNotShowPos())
      , UseTotalCountersAsShows(Scheme().Params().UseTotalCountersAsShows())
      , WarmupDuration(Scheme().Params().WarmupDuration())
      , ShowsAddition(Scheme().Params().ShowsAddition())
      , ShowsMultiplier(Scheme().Params().ShowsMultiplier())
      , AdditionalSeed(Scheme().Params().AdditionalSeed()) {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);
        if (CountersMode != ECountersMode::FromFeatures) {
            GetMeta().RegisterAttr(NMeta::RAW_PREDICTIONS);
        }
    }

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        if (!Scheme().WarmupRandomizeBundle().IsNull()) {
            WarmupRandomizeBundle = LoadFormulaProto(Scheme().WarmupRandomizeBundle(), formulasStoragePtr);
            Y_ENSURE(!WarmupRandomizeBundle || (WarmupRandomizeBundle->GetNumFeats() == 0));
        } else {
            Y_ENSURE(WarmupDuration <= 0);
        }
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "thompson sampling bundle calculation started: " << GetAlias() << '\n';
        size_t positionalFeaturesStart = 0;
        float totalShows = 0.0;
        if (WarmupRandomizeBundle || UseTotalCountersAsShows) {
            totalShows = features[0];
            positionalFeaturesStart = 1;
            context.DbgLog() << "totalShows: " << totalShows << '\n';
        }
        if (WarmupRandomizeBundle) {
            if (totalShows < WarmupDuration && (CountersMode == ECountersMode::FromFeatures || GetFormulaScores(context).IsNull())) {
                context.DbgLog() << "use warmup distribution" << '\n';
                LogValue(context, "use_warmup_distribution", true);
                return WarmupRandomizeBundle->DoCalcRelevExtended(nullptr, 0, context);
            }
        }
        LogParameters(context);
        const auto& seedStr = TString{context.GetMeta().RandomSeed().Get()} + AdditionalSeed;
        NExtendedMx::TRandom random(seedStr);
        size_t bestPos = ChoosePos(context, features + positionalFeaturesStart, seedStr, random, totalShows);
        context.GetResult().FeatureResult()[PosFeatureName].Result()->SetIntNumber(bestPos);
        LogValue(context, FEATURE_LOG_PREF + PosFeatureName, bestPos);

        // choose other features (ViewType, Place, etc) randomly with uniform distribution
        RandomSelectAdditionalFeatures(*this, Scheme().Params().AdditionalFeatures(), random, context);
        return bestPos;
    }
private:
    TAllCategScoresConstProto GetFormulaScores(const TCalcContext& context) const {
        return context.GetMeta().RawPredictions().AllCategScores();
    }

    void ProcessPos(
            TCalcContext& context, const TDistributionHolder& distribution, size_t pos, const TString& seedStr,
            size_t& bestPos, float& bestScore) const {
        const TString currPosSeed = seedStr + "_" + ToString(pos);
        const ui32 seed = MurmurHash<ui32>(currPosSeed.c_str(), currPosSeed.size());
        auto rng = TFastRng<ui32>(seed);
        float score = distribution->Sample(rng);
        context.DbgLog() << "pos " << pos << " has score " << Prec(score, 5) << "\n";
        if (score > bestScore) {
            bestPos = pos;
            bestScore = score;
        }
    }

    size_t ChoosePos(TCalcContext& context, const float* features, const TString& seedStr, TRandom& random, float totalShows) const {
        size_t bestPos = DoNotShowPos;
        float bestScore = ShowScoreThreshold;
        const bool uniform = random.NextReal(1.0) < UniformProba;
        if (uniform) {
            bestPos = random.NextInt(PositionNum + 1);
            if (bestPos == PositionNum) {
                bestPos = DoNotShowPos;
            }
        }
        for (size_t pos = 0; pos < PositionNum; ++pos) {
            const auto counters = GetSurplusCounters(features, pos, context, totalShows);
            LogSurplusCounters(context, counters, pos);
            if (!uniform) {
                ProcessPos(context, BuildDistribution(counters, Distribution), pos, seedStr, bestPos, bestScore);
            }
        }
        context.DbgLog() << "pos " << bestPos << " has been chosen ";
        if (uniform) {
            context.DbgLog() << "via uniform random" << '\n';
        } else {
            context.DbgLog() << "with score " << bestScore << '\n';
        }
        return bestPos;
    }

    void LogParameters(TCalcContext& context) const {
        const TString prefix = "random_param_";
        LogValue(context, prefix + "show_score_threshold", ShowScoreThreshold);
        LogValue(context, prefix + "uniform_proba", UniformProba);
        LogValue(context, prefix + "distribution_name", DistributionName);
    }

    void LogSurplusCounters(TCalcContext& context, const SurplusCounters& counters, size_t pos) const {
        TString prefix = FEATURE_LOG_PREF + PosFeatureName + "_" + ToString(pos) + "_";
        LogValue(context, prefix + "win", counters.win);
        LogValue(context, prefix + "loss", counters.loss);
        LogValue(context, prefix + "shows", counters.shows);
    }

    SurplusCounters GetSurplusCounters(const float* features, size_t pos, const TCalcContext& context, float totalShows) const {
        SurplusCounters counters = GetCountersFromFeatures(features, pos);
        if (CountersMode != ECountersMode::FromFeatures) {
            const auto& formulaScores = GetFormulaScores(context);
            if (!formulaScores.IsNull()) {
                if (UseTotalCountersAsShows) {
                    counters.shows = totalShows;
                }
                const float shows = counters.shows * ShowsMultiplier + ShowsAddition;
                if (CountersMode == ECountersMode::FromSubtargets) {
                    counters = GetCountersFromSubtargetScores(formulaScores, pos, shows);
                } else if (CountersMode == ECountersMode::FromUpper) {
                    counters = GetCountersFromUpperScores(formulaScores, pos, shows);
                } else {
                    ythrow yexception() << "unknown counters mode: " << CountersMode;
                }
            }
        }
        return counters;
    }

    float CalcSigmoid(float x) const {
        return 1 / (1 + std::exp(-x));
    }

    SurplusCounters GetCountersFromSubtargetScores(const TAllCategScoresConstProto& scores, size_t pos, float showsFromFeatures) const {
        const float win = CalcSigmoid(scores["sub_win"][pos]) * showsFromFeatures;
        const float loss = CalcSigmoid(scores["sub_loss"][pos]) * showsFromFeatures;
        return SurplusCounters(win, loss, showsFromFeatures);
    }

    SurplusCounters GetCountersFromUpperScores(const TAllCategScoresConstProto& scores, size_t pos, float showsFromFeatures) const {
        float win = scores["upper_pure_win"][pos] * ShowScoreThreshold;
        float surplus = scores["upper_pure_surplus"][pos] * ShowScoreThreshold;
        return SurplusCounters(win, win - surplus, showsFromFeatures);
    }

    SurplusCounters GetCountersFromFeatures(const float* features, size_t pos) const {
        const float shows = features[3 * pos];
        const float surplus = features[3 * pos + 1];
        const float winPerShow = features[3 * pos + 2];
        const float win = winPerShow * shows;
        const float loss = win - surplus;
        return SurplusCounters(win, loss, shows);
    }

    size_t GetNumFeats() const override {
        // need three factros for each of 10 positions and 1 total shows factor
        return WarmupRandomizeBundle || UseTotalCountersAsShows ? 31 : 30;
    }

private:
    const float ShowScoreThreshold;
    const TString DistributionName;
    const EDistribution Distribution;
    const ECountersMode CountersMode;
    const float UniformProba;
    const size_t DoNotShowPos;
    const bool UseTotalCountersAsShows;
    const size_t PositionNum = 10;
    const float WarmupDuration;
    const float ShowsAddition;
    const float ShowsMultiplier;
    const TString AdditionalSeed;
    const TString PosFeatureName = "Pos";
    TExtendedRelevCalcer* WarmupRandomizeBundle = nullptr;
};


TExtendedCalculatorRegistrator<TThompsonSamplingBundle> ThompsonSamplingBundleRegistrator("thompson_sampling");
