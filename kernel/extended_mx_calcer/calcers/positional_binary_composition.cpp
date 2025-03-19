#include "bundle.h"
#include "clickint.h"

#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/string/printf.h>

#include <math.h>

using namespace NExtendedMx;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TPositionalBinaryCompositionBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TPositionalBinaryCompositionBundle : public TBundleBase<TPositionalBinaryCompositionConstProto> {
public:
    TPositionalBinaryCompositionBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        NumFeats = 0;

        for (const auto& p : Scheme().Predictors()) {
            auto calcer = LoadFormulaProto(p.Calcer(), formulasStoragePtr);
            if (calcer->GetNumFeats() > NumFeats) {
                NumFeats = calcer->GetNumFeats();
            }
            Predictors.push_back({calcer, p});
        }
        Validate();
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "PositionalBinaryComposition calculation started\n";
        const auto& params = Scheme().Params();
        double bestScore = params.NotShownScore();
        size_t bestPos = params.NotShownPosition();
        for (const auto& p : Predictors) {
            double score = p.CalcerPtr->DoCalcRelevExtended(features, featuresCount, context);
            if (p.Params.DoSigmoid()) {
                score = Sigmoid(score);
            }
            score = score * p.Params.Multiplier() + p.Params.Bias();
            if (score > bestScore) {
                bestScore = score;
                bestPos = p.Params.Position();
            }
            if (params.LogPositionScore()) {
                LogValue(context, "score_pos_" + ToString(p.Params.Position()), ToString(score));
            }
            context.DbgLog() << "Position=" << p.Params.Position() << ", Score=" << score << "\n";
        }
        context.GetResult().FeatureResult()[params.PosFeatureName()].Result()->SetIntNumber(bestPos);
        LogValue(context, FEATURE_LOG_PREF + ToString(params.PosFeatureName()), bestPos);
        return bestScore;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    void Validate() const {
        Y_ENSURE(!Predictors.empty(), "Empty predictors");
        THashSet<size_t> positions;
        for (const auto& p: Predictors) {
            Y_ENSURE(!positions.contains(p.Params.Position()), Sprintf("Duplicate position [%u]", p.Params.Position().Get()));
            positions.insert(p.Params.Position());
        }
    }

private:
    struct TPredictor {
        TExtendedRelevCalcer* CalcerPtr;
        TPositionalBinaryCompositionConstProto::TPositionPredictorConst Params;
    };

    TVector<TPredictor> Predictors;
    size_t NumFeats = 0;
};


TExtendedCalculatorRegistrator<TPositionalBinaryCompositionBundle> PosBinCompositionRegistrator("positional_bin_composition");
