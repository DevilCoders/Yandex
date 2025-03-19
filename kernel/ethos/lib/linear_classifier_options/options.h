#pragma once

#include <library/cpp/getopt/last_getopt.h>
#include <util/ysaveload.h>

namespace NEthos {

enum ETransductiveMode {
    NO_TRANSDUCTIVE_LEARNING,
    TRANSDUCTIVE_RECALL,
    TRANSDUCTIVE_PRECISION,
};

enum EStratificationMode {
    NO_STRATIFICATION,
    LABEL_STRATIFICATION,
};

enum EThresholdMode {
    NO_THRESHOLD_SELECTION,
    SELECT_BEST_THRESHOLD,
};

enum EFeatureGenerationMode {
    OUT_OF_GROWING_FOLD,
    OUT_OF_FOLD,
};

enum ELearningMethod {
    LM_LOGISTIC_REGRESSION,
    LM_NAIVE_BAYES,
};

struct TLinearClassifierOptions {
    size_t IterationsCount = 100000;
    size_t IterationsCountMultiplier = 0;

    double Shrinkage = 0.01;

    double PositivesOffset = 100.;
    double NegativesOffset = 100.;

    double PositivesFactor = 1.;
    double NegativesFactor = 1.;

    ETransductiveMode TransductiveMode = NO_TRANSDUCTIVE_LEARNING;

    size_t UnmarkedCount = 1;
    double UmarkedFactor = 0.05;

    EStratificationMode StratificationMode = LABEL_STRATIFICATION;

    EThresholdMode ThresholdMode = NO_THRESHOLD_SELECTION;

    size_t FeaturesToUseCount = 50000;
    float WeightsLowerBound = 0.1f;

    size_t ThreadCount = 1;

    EFeatureGenerationMode FeatureGenerationMode = OUT_OF_GROWING_FOLD;
    size_t FeatureFoldsCount = 10;

    double BayesRegularizationParameter = 0.01;
    size_t BayesIterationsCount = 0;

    ELearningMethod LearningMethod = LM_LOGISTIC_REGRESSION;

    bool LearnBayesOnNegatives = false;

    bool ReWeightStratification = false;

    Y_SAVELOAD_DEFINE(IterationsCount,
                    IterationsCountMultiplier,
                    Shrinkage,
                    PositivesOffset,
                    NegativesOffset,
                    PositivesFactor,
                    NegativesFactor,
                    TransductiveMode,
                    UnmarkedCount,
                    UmarkedFactor,
                    StratificationMode,
                    ThresholdMode,
                    FeaturesToUseCount,
                    WeightsLowerBound,
                    ThreadCount,
                    FeatureGenerationMode,
                    FeatureFoldsCount,
                    BayesRegularizationParameter,
                    BayesIterationsCount,
                    LearningMethod,
                    LearnBayesOnNegatives,
                    ReWeightStratification);

    void AddOpts(NLastGetopt::TOpts& opts) {
        opts.AddCharOption('i', "iterations count")
            .Optional()
            .StoreResult(&IterationsCount);
        opts.AddCharOption('m', "iterations count multiplier; pverwrites -i option")
            .Optional()
            .StoreResult(&IterationsCountMultiplier);

        opts.AddCharOption('w', "shrinkage")
            .Optional()
            .StoreResult(&Shrinkage);

        opts.AddLongOption("positives-offset", "positives offset")
            .Optional()
            .StoreResult(&PositivesOffset);
        opts.AddLongOption("negatives-offset", "negatives offset")
            .Optional()
            .StoreResult(&NegativesOffset);

        opts.AddLongOption("positives-factor", "factor for positive instances")
            .Optional()
            .StoreResult(&PositivesFactor);
        opts.AddLongOption("negatives-factor", "factor for negatives instances")
            .Optional()
            .StoreResult(&NegativesFactor);

        opts.AddLongOption("transductive-recall", "improve recall using unmarked instances")
            .Optional()
            .NoArgument()
            .StoreValue(&TransductiveMode, TRANSDUCTIVE_RECALL);
        opts.AddLongOption("transductive-precision", "improve precision using unmarked instances")
            .Optional()
            .NoArgument()
            .StoreValue(&TransductiveMode, TRANSDUCTIVE_PRECISION);

        opts.AddLongOption("unmarked-count", "number of unmarked items used at each iteration")
            .Optional()
            .StoreResult(&UnmarkedCount);
        opts.AddLongOption("unmarked-factor", "factor for unmarked items")
            .Optional()
            .StoreResult(&UmarkedFactor);

        opts.AddLongOption("no-stratification", "turn off stratification")
            .Optional()
            .NoArgument()
            .StoreValue(&StratificationMode, NO_STRATIFICATION);

        opts.AddLongOption("select-best-threshold", "turn on selecting best threshold")
            .Optional()
            .NoArgument()
            .StoreValue(&ThresholdMode, SELECT_BEST_THRESHOLD);

        opts.AddLongOption("select-features", "number of features to select before learning")
            .Optional()
            .StoreResult(&FeaturesToUseCount);

        opts.AddLongOption("weights-lower-bound", "lower bound for saving weights")
            .Optional()
            .StoreResult(&WeightsLowerBound);

        // XXX: redesign
        opts.AddLongOption("learn-threads", "thread count for multi-label learning")
            .Optional()
            .StoreResult(&ThreadCount);

        opts.AddLongOption("out-of-growing-fold", "out-of-growing-fold mode for features generation")
            .Optional()
            .NoArgument()
            .StoreValue(&FeatureGenerationMode, EFeatureGenerationMode::OUT_OF_GROWING_FOLD);
        opts.AddLongOption("out-of-fold", "out-of-fold mode for features generation")
            .Optional()
            .StoreValue(&FeatureGenerationMode, EFeatureGenerationMode::OUT_OF_FOLD)
            .StoreResult(&FeatureFoldsCount);

        opts.AddLongOption("bayes", "learn naive Bayes classifier")
            .Optional()
            .NoArgument()
            .StoreValue(&LearningMethod, ELearningMethod::LM_NAIVE_BAYES);
        opts.AddLongOption("bayes-regularization", "regularization parameter for Bayes methods")
            .Optional()
            .StoreResult(&BayesRegularizationParameter);
        opts.AddLongOption("bayes-iterations", "Bayes improve iterations count")
            .Optional()
            .StoreResult(&BayesIterationsCount);

        opts.AddLongOption("re-weight", "re-weight samples for stratified learning")
            .Optional()
            .NoArgument()
            .StoreValue(&ReWeightStratification, true);
    }

    size_t GetIterationsCount(const size_t sampleSize) const {
        return IterationsCountMultiplier ? sampleSize * IterationsCountMultiplier : IterationsCount;
    }
};

struct TApplyOptions {
    bool NormalizeOnLength = false;

    size_t MaxTokens = 0;
    size_t MinTokens = 0;

    void AddOpts(NLastGetopt::TOpts& opts) {
        opts.AddLongOption("normalize-on-length")
            .Optional()
            .NoArgument()
            .StoreValue(&NormalizeOnLength, true);

        opts.AddLongOption("max-tokens", "number of tokens with maximal prediction")
            .Optional()
            .StoreResult(&MaxTokens);
        opts.AddLongOption("min-tokens", "number of tokens with maximal prediction")
            .Optional()
            .StoreResult(&MinTokens);
    }
};

}
