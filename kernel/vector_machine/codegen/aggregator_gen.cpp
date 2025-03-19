#include "common.h"

#include <kernel/vector_machine/aggregators.h>

#include <util/generic/yexception.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NVectorMachine {
    namespace {
        TVector<TString> GenerateTopSmthFeaturesNames(const TAggregatorConfig& config, const TString& prefix, const TString& smth, const TString& suffix = {}) {
            TVector<TString> names;
            for (float param : config.GetParams()) {

                Y_ENSURE(param >= 0.f, "Cannot set a negative param.");

                if (param == 0.f) {
                    names.push_back(TString::Join("Max", smth, suffix));
                } else if (param == 1.f) {
                    names.push_back(TString::Join(prefix, smth, suffix));
                } else if (param > 0.f && param < 1.f) {
                    names.push_back(TString::Join(prefix, "Top", ToString(100.f * param), "p", smth, suffix));
                } else { // if param > 1.f
                    size_t paramRounded = static_cast<size_t>(param);
                    Y_ENSURE(static_cast<float>(paramRounded) == param, "Cannot set a float param out of range [0, 1].");
                    names.push_back(TString::Join(prefix, "Top", ToString(paramRounded), smth, suffix));
                }
            }
            return names;
        }

        TVector<TString> GenerateScoreThresholdAggregatorFeaturesNames(const TAggregatorConfig& config) {
            TVector<TString> names;
            NAggregatorUtils::EThresholdStatTypes stats;

            for (const int statType : config.GetThresholdStats()) {
                stats |= static_cast<NAggregatorUtils::EThresholdStatType>(statType);
            }

            for (float th : config.GetParams()) {
                th *= 100;
                if (stats & EThresholdStatType::TST_MAX_WEIGHT) {
                    names.push_back(TString::Join("ScoreThreshold", ToString(th), "pMaxWeight"));
                }
                if (stats & EThresholdStatType::TST_AVG_WEIGHT) {
                    names.push_back(TString::Join("ScoreThreshold", ToString(th), "pAvgWeight"));
                }
                if (stats & EThresholdStatType::TST_REL_COUNT) {
                    names.push_back(TString::Join("ScoreThreshold", ToString(th), "pRelCount"));
                }
                if (stats & EThresholdStatType::TST_COUNT) {
                    names.push_back(TString::Join("ScoreThreshold", ToString(th), "pCount"));
                }
                if (stats & EThresholdStatType::TST_SATURATED_WEIGHT_SUM) {
                    names.push_back(TString::Join("ScoreThreshold", ToString(th), "pSaturatedWeightSum"));
                }
            }

            return names;
        }

        TVector<TString> GenerateIdentityNames(const TAggregatorConfig& config) {
            TVector<TString> names;
            for (const float param : config.GetParams()) {
                const size_t roundedParam = static_cast<size_t>(param);
                names.push_back(TString::Join("Value", ToString(roundedParam)));
            }
            return names;
        }

        TVector<TString> GenerateFeatureNames(const TString& prefix, const TAggregatorConfig& config) {
            TVector<TString> names;
            switch (config.GetType()) {
                case EAggregatorType::AT_AVG_TOP_SCORE:
                    names = GenerateTopSmthFeaturesNames(config, "Avg", "Score");
                    break;
                case EAggregatorType::AT_AVG_TOP_SCORE_WEIGHTED:
                    names = GenerateTopSmthFeaturesNames(config, "Avg", "Score", "Weighted");
                    break;
                case EAggregatorType::AT_MIN_TOP_SCORE:
                    names = GenerateTopSmthFeaturesNames(config, "Min", "Score");
                    break;
                case EAggregatorType::AT_MIN_TOP_SCORE_WEIGHTED:
                    names = GenerateTopSmthFeaturesNames(config, "Min", "Score", "Weighted");
                    break;
                case EAggregatorType::AT_SCORE_THRESHOLD:
                    names = GenerateScoreThresholdAggregatorFeaturesNames(config);
                    break;
                case EAggregatorType::AT_AVG_TOP_WEIGHT:
                    names = GenerateTopSmthFeaturesNames(config, "Avg", "Weight");
                    break;
                case EAggregatorType::AT_MIN_TOP_WEIGHT:
                    names = GenerateTopSmthFeaturesNames(config, "Min", "Weight");
                    break;
                case EAggregatorType::AT_IDENTITY:
                    names = GenerateIdentityNames(config);
                    break;
                case EAggregatorType::AT_AVG_TOP_SCORE_X_WEIGHT:
                    names = GenerateTopSmthFeaturesNames(config, "Avg", "ScoreXWeight");
                    break;
                default:
                    ythrow yexception() << "Unknown aggregator type";
            }

            if (prefix) {
                for (TString& featureName : names) {
                    featureName = TString::Join(prefix, "_", featureName);
                }
            }

            return names;
        }
    }

    TGeneratedAggregator::TGeneratedAggregator(const TAggregatorConfig& config, const TString& name, const TString& featurePrefix)
        : Config_(config)
        , Name_(name)
    {
        FeatureNames_ = GenerateFeatureNames(featurePrefix, config);

        switch (config.GetType()) {
            case EAggregatorType::AT_AVG_TOP_SCORE:
                ClassName_ = "TAvgTopScoreAggregator";
                break;
            case EAggregatorType::AT_AVG_TOP_SCORE_WEIGHTED:
                ClassName_ = "TAvgTopScoreWeightedAggregator";
                break;
            case EAggregatorType::AT_MIN_TOP_SCORE:
                ClassName_ = "TMinTopScoreAggregator";
                break;
            case EAggregatorType::AT_MIN_TOP_SCORE_WEIGHTED:
                ClassName_ = "TMinTopScoreWeightedAggregator";
                break;
            case EAggregatorType::AT_SCORE_THRESHOLD:
                ClassName_ = GenerateNameForScoreThresholdAggregator(config);
                break;
            case EAggregatorType::AT_AVG_TOP_WEIGHT:
                ClassName_ = "TAvgTopWeightAggregator";
                break;
            case EAggregatorType::AT_MIN_TOP_WEIGHT:
                ClassName_ = "TMinTopWeightAggregator";
                break;
            case EAggregatorType::AT_IDENTITY:
                ClassName_ = "TIdentityAggregator";
                break;
            case EAggregatorType::AT_AVG_TOP_SCORE_X_WEIGHT:
                ClassName_ = "TAvgTopScoreXWeightAggregator";
                break;
            default:
                ythrow yexception() << "Unknown aggregator type";
        }
    }

    TString TGeneratedAggregator::GenerateNameForScoreThresholdAggregator(const TAggregatorConfig& config) const {
        TStringBuilder result;
        result << "TScoreThresholdAggregator<NAggregatorUtils::EThresholdStatTypes()";
        for (const int statType : config.GetThresholdStats()) {
            result << " | NAggregatorUtils::" << EThresholdStatType_Name(static_cast<EThresholdStatType>(statType));
        }
        result << ">";
        return result;
    }

    void TGeneratedAggregator::OutputParamsList(IOutputStream& out) const {
        out << "{";
        for (float param : Config_.GetParams()) {
            out << "static_cast<float>(" << param << "), ";
        }
        out << "}";
    }
};
