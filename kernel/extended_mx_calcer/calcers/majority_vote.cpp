#include "bundle.h"
#include "clickint.h"
#include "random.h"

#include <util/generic/ymath.h>
#include <library/cpp/string_utils/base64/base64.h>


using namespace NExtendedMx;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TPositionalMajorityVoteBundle - calculate subtargets as .info formulas, then calc decision trees
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TPositionalMajorityVoteBundle : public TBundleBase<TPositionalMajorityVoteConstProto> {
    using TRelevCalcer = NMatrixnet::IRelevCalcer;
    using TRelevCalcers = TVector<TRelevCalcer*>;

    struct TFeatureVotes {
        TVector<double> Borders;
        TVector<TVector<double>> Votes;             // Votes[border][classId]
    };

public:
    TPositionalMajorityVoteBundle(const NSc::TValue& scheme)
      : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        FeatureName = Scheme().Params().FeatureName();
        NumFeats = 0;

        Y_ENSURE(FeatureName, "required feature name");

        const auto& targetFmls = Scheme().Subtargets();
        const size_t targetsCount = targetFmls.Size();
        Subtargets.reserve(targetsCount);
        for (const auto& target : targetFmls) {
            auto sub = LoadFormulaProto<TRelevCalcer>(target, formulasStoragePtr);
            NumFeats = Max(NumFeats, sub->GetNumFeats());
            Subtargets.push_back(sub);
        }
        Positions.reserve(Scheme().Params().Positions().Size());
        for (const auto &pos : Scheme().Params().Positions())
            Positions.push_back(pos);
        Y_ENSURE(Positions.size() > 1, "should be more then one position");

        const auto &featuresVotes = Scheme().Voters();
        Y_ENSURE(featuresVotes.Size() == targetsCount, "number of voters should be equal to number of subtargets");
        FeaturesVotes.reserve(targetsCount);
        for (const auto &feature : featuresVotes) {
            FeaturesVotes.push_back(TFeatureVotes());
            TFeatureVotes &dst = FeaturesVotes.back();
            dst.Borders.reserve(feature.Borders().Size());
            for (const auto &border : feature.Borders()) {
                dst.Borders.push_back(border);
            }
            dst.Votes.reserve(feature.BorderVotes().Size());
            for (const auto &votes : feature.BorderVotes()) {
                Y_ENSURE(votes.Answers().Size() == Positions.size(), "number of votes should be equal to number of positions");
                dst.Votes.push_back(TVector<double>());
                TVector<double> &ans = dst.Votes.back();
                ans.reserve(votes.Answers().Size());
                for (const auto &val : votes.Answers()) {
                    ans.push_back(val);
                }
            }
            Y_ENSURE(dst.Borders.size() + 1 == dst.Votes.size(), "number of border votes should be equal to number of borders + 1");
        }
    }

    void CalcSubtargets(const float *features, const size_t featuresCount, TCalcContext& context, TVector<TVector<double>> &result) const {
        TFactors factors(1, TVector<float>(features, features + featuresCount));
        DebugFactorsDump("features", factors, context);
        CalcMultiple(Subtargets, factors, result);
    }

    size_t BorderIndex(const TVector<double> &borders, double value) const {
        return std::lower_bound(borders.begin(), borders.end(), value) - borders.begin();
    }

public:
    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "positional majoriry vote calculation started: " << GetAlias() << "\n";
        TVector<TVector<double>> subResults;
        CalcSubtargets(features, featuresCount, context, subResults);
        TVector<double> factors, votes(Positions.size());
        factors.reserve(subResults.size());
        for (size_t i = 0, cnt = subResults.size(); i < cnt; ++i) {
            const TFeatureVotes &featureVotes = FeaturesVotes[i];
            size_t idx = BorderIndex(featureVotes.Borders, subResults[i].front());
            for (size_t j = 0; j < votes.size(); ++j) {
                votes[j] += featureVotes.Votes[idx][j];
            }
        }
        size_t res = 0;
        for (int i = votes.ysize() - 1; i > 0; --i) {
            if (votes[i] >= votes[res])
                res = i;
        }

        if (context.DbgLog().IsEnabled()) {
            context.DbgLog() << "votes:         " << JoinSeq("\t", votes) << '\n';
            context.DbgLog() << "positions:     " << JoinSeq("\t", Positions) << '\n';
            context.DbgLog() << "selected item: " << res << '\n';
            context.DbgLog() << "selected pos:  " << Positions[res] << '\n';
        }
        ProcessStepAsPos(*this, context, Positions[res], FeatureName);
        return Positions[res];
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    TRelevCalcers Subtargets;
    TString FeatureName;
    TVector<i32> Positions;
    TVector<TFeatureVotes> FeaturesVotes;
    size_t NumFeats = 0;
};

TExtendedCalculatorRegistrator<TPositionalMajorityVoteBundle> PositionalMajorityVoteBundleRegistrator("positional_majority_vote");

