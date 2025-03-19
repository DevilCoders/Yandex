#include <kernel/ethos/lib/cross_validation/cross_validation.h>
#include <kernel/ethos/lib/data/dataset.h>

#include <kernel/ethos/lib/linear_classifier_options/options.h>
#include <kernel/ethos/lib/linear_model/binary_model.h>

#include <kernel/ethos/lib/logistic_regression/logistic_regression.h>
#include <kernel/ethos/lib/naive_bayes/naive_bayes.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <library/cpp/linear_regression/linear_regression.h>
#include <library/cpp/linear_regression/linear_model.h>
#include <library/cpp/linear_regression/welford.h>

#include <library/cpp/scheme/scheme.h>

#include <library/cpp/streams/factory/factory.h>

#include <util/string/join.h>
#include <util/string/printf.h>

#include <util/system/fs.h>
#include <util/system/yassert.h>

void PrintMetrics(const NSc::TValue& metricsMap, const TString& mainMetric = "") {
    TVector<TString> metricNames;
    for (auto&& metricWithValue : metricsMap.GetDict()) {
        metricNames.push_back(TString{metricWithValue.first});
    }
    Sort(metricNames.begin(), metricNames.end());

    for (TString& name : metricNames) {
        if (mainMetric == name) {
            std::swap(metricNames.front(), name);
        }
    }

    for (const TString& metricName : metricNames) {
        const double value = metricsMap[metricName].GetNumber();
        TString metricToPrint = metricName;
        while (metricToPrint.size() < 12) {
            metricToPrint += ' ';
        }
        Cout << "  " << metricToPrint << Sprintf("%.5lf", value) << Endl;
        if (metricName == mainMetric) {
            Cout << Endl;
        }
    }
}

struct TRunOptions {
    TString FeaturesPath;
    TString ModelPath;

    NEthos::TLinearClassifierOptions LinearClassifierOptions;
    NEthos::TApplyOptions ApplyOptions;

    void AddOpts(NLastGetopt::TOpts& opts) {
        opts.AddLongOption('f', "features", "learn features file path")
            .Required()
            .StoreResult(&FeaturesPath);
        opts.AddLongOption("model", "model path")
            .Optional()
            .StoreResult(&ModelPath);

        LinearClassifierOptions.AddOpts(opts);
        ApplyOptions.AddOpts(opts);
    }
};

template <typename TLinearClassifierLearner>
NEthos::TBinaryClassificationLinearModel LearnLinearModelTemplate(const NEthos::TBinaryLabelFloatFeatureVectors& pool, const TRunOptions& runOptions) {
    TLinearClassifierLearner learner(runOptions.LinearClassifierOptions);
    return learner.Learn(pool.begin(), pool.end());
}

NEthos::TBinaryClassificationLinearModel LearnLinearModel(const NEthos::TBinaryLabelFloatFeatureVectors& pool, const TRunOptions& runOptions) {
    switch (runOptions.LinearClassifierOptions.LearningMethod) {
    case NEthos::ELearningMethod::LM_LOGISTIC_REGRESSION: return LearnLinearModelTemplate<NEthos::TBinaryLabelLogisticRegressionLearner>(pool, runOptions);
    case NEthos::ELearningMethod::LM_NAIVE_BAYES: return LearnLinearModelTemplate<NEthos::TBinaryLabelNaiveBayesLearner>(pool, runOptions);
    }

    return NEthos::TBinaryClassificationLinearModel();
}

NEthos::TBcMetrics Metrics(const NEthos::TBinaryLabelFloatFeatureVectors& pool,
                           const NEthos::TBinaryClassificationLinearModel& linearModel,
                           const TRunOptions& runOptions)
{
    NEthos::TBcMetricsCalculator metricsCalculator;
    for (const NEthos::TBinaryLabelFloatFeatureVector& instance : pool) {
        metricsCalculator.Add(linearModel.Apply(instance.Features, &runOptions.ApplyOptions), instance.Label, instance.Weight);
    }
    return metricsCalculator.AllMetrics();
}

NEthos::TBcMetrics Metrics(const NEthos::TBinaryLabelFloatFeatureVectors& pool,
    const NEthos::TFloatFeatureWeightMap& weights,
    const TRunOptions& runOptions)
{
    NEthos::TBcMetricsCalculator metricsCalculator;
    for (const NEthos::TBinaryLabelFloatFeatureVector& instance : pool) {
        const double prediction = NEthos::LinearPrediction(weights, instance.Features, &runOptions.ApplyOptions);
        const NEthos::EBinaryClassLabel label = NEthos::BinaryLabelFromPrediction(prediction, 0.);

        const NEthos::TBinaryLabelWithFloatPrediction labelWithPrediction(label, prediction);
        metricsCalculator.Add(labelWithPrediction, instance.Label, instance.Weight);
    }
    return metricsCalculator.AllMetrics();
}

int DoLearn(int argc, const char** argv) {
    TRunOptions runOptions;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        runOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    NEthos::TBinaryLabelFloatFeatureVectors pool = NEthos::TBinaryLabelFloatFeatureVectors::FromFeatures(runOptions.FeaturesPath);
    NEthos::TBinaryClassificationLinearModel model = LearnLinearModel(pool, runOptions);

    if (runOptions.ModelPath) {
        TFixedBufferFileOutput modelOut(runOptions.ModelPath);
        model.Save(&modelOut);
    }

    NEthos::TBcMetrics metrics = Metrics(pool, model, runOptions);

    PrintMetrics(metrics.ToTValue(), "F1");

    return 0;
}

struct TTargetWithPrediction {
    double Target;
    double Prediction;

    bool operator < (const TTargetWithPrediction& other) const {
        if (Prediction != other.Prediction) {
            return Prediction > other.Prediction;
        }

        return Target < other.Target;
    }
};

double BadRankedPairsFraction(const TVector<TTargetWithPrediction>& sortedTargetsAndPredictions) {
    TMap<double, size_t> targetToCount;
    for (const TTargetWithPrediction& targetWithPrediction : sortedTargetsAndPredictions) {
        ++targetToCount[targetWithPrediction.Target];
    }

    size_t totalPairsCount = 0;
    {
        for (auto&& targetWithCount : targetToCount) {
            size_t othersCount = sortedTargetsAndPredictions.size() - targetWithCount.second;
            totalPairsCount += othersCount * targetWithCount.second;
        }
        totalPairsCount /= 2;
    }

    size_t badPairsCount = 0;
    for (const TTargetWithPrediction& targetWithPrediction : sortedTargetsAndPredictions) {
        for (TMap<double, size_t>::const_iterator bound = targetToCount.upper_bound(targetWithPrediction.Target); bound != targetToCount.end(); ++bound) {
            badPairsCount += bound->second;
        }

        size_t& targetCount = targetToCount[targetWithPrediction.Target];
        if (targetCount == 1) {
            targetToCount.erase(targetWithPrediction.Target);
        } else {
            --targetCount;
        }
    }

    return (double) badPairsCount / totalPairsCount;
}

double BadRankedPairsFraction(const NEthos::TBinaryLabelFloatFeatureVectors& pool, const NEthos::TFloatFeatureWeightMap& weights) {
    TVector<TTargetWithPrediction> targetsAndPredictions;
    for (const NEthos::TBinaryLabelFloatFeatureVector& instance : pool) {
        const double prediction = NEthos::LinearPrediction(weights, instance.Features);
        const double target = instance.OriginalTarget;
        targetsAndPredictions.push_back({ target, prediction });
    }
    Sort(targetsAndPredictions.begin(), targetsAndPredictions.end());

    return BadRankedPairsFraction(targetsAndPredictions);
}

int DoPairwiseLearn(int argc, const char** argv) {
    TRunOptions runOptions;

    size_t epochCount = 10;
    size_t pairsOverhead = 3;

    bool doPrintPairwiseMetrics = true;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("epochs")
            .Optional()
            .StoreResult(&epochCount);
        opts.AddLongOption("overhead")
            .Optional()
            .StoreResult(&pairsOverhead);

        runOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    runOptions.LinearClassifierOptions.StratificationMode = NEthos::EStratificationMode::NO_STRATIFICATION;
    runOptions.LinearClassifierOptions.FeaturesToUseCount = 0;

    NEthos::TBinaryLabelFloatFeatureVectors pool = NEthos::TBinaryLabelFloatFeatureVectors::FromFeatures(runOptions.FeaturesPath);

    THashSet<double> differentTargets;

    TVector<size_t> instanceNumbers;
    for (size_t instanceNumber = 0; instanceNumber < pool.size(); ++instanceNumber) {
        instanceNumbers.push_back(instanceNumber);

        if (differentTargets.size() < 10000) {
            differentTargets.insert(pool[instanceNumber].OriginalTarget);
        }
    }

    if (differentTargets.size() == 10000) {
        Cerr << "too many different targets; will not calculate pairwise metrics" << Endl;
        doPrintPairwiseMetrics = false;
    }

    NEthos::TFloatFeatureWeightMap weights;

    TMersenne<ui64> mersenne;
    for (size_t epoch = 0; epoch < epochCount; ++epoch) {
        NEthos::TBinaryLabelFloatFeatureVectors currentPool;

        {
            Shuffle(instanceNumbers.begin(), instanceNumbers.end(), mersenne);
            for (size_t instanceIdx = 0; instanceIdx < pool.size(); ++instanceIdx) {
                const NEthos::TBinaryLabelFloatFeatureVector& firstInstance = pool[instanceNumbers[instanceIdx]];
                for (size_t neighbourOffset = 1; neighbourOffset <= pairsOverhead; ++neighbourOffset) {
                    const NEthos::TBinaryLabelFloatFeatureVector& secondInstance = pool[instanceNumbers[(instanceIdx + neighbourOffset) % pool.size()]];
                    if (firstInstance.OriginalTarget == secondInstance.OriginalTarget) {
                        continue;
                    }

                    THashMap<ui64, float> featuresMap;
                    for (const NEthos::TIndexedFloatFeature& feature : firstInstance.Features) {
                        featuresMap[feature.Index] += feature.Value;
                    }
                    for (const NEthos::TIndexedFloatFeature& feature : secondInstance.Features) {
                        featuresMap[feature.Index] -= feature.Value;
                    }

                    NEthos::TFloatFeatureVector featuresVector;
                    for (auto&& feature : featuresMap) {
                        featuresVector.push_back(NEthos::TIndexedFloatFeature(feature.first, feature.second));
                    }

                    const NEthos::EBinaryClassLabel label = firstInstance.OriginalTarget > secondInstance.OriginalTarget
                        ? NEthos::EBinaryClassLabel::BCL_POSITIVE
                        : NEthos::EBinaryClassLabel::BCL_NEGATIVE;

                    const double weight = log(fabs(firstInstance.OriginalTarget - secondInstance.OriginalTarget) + 1.);

                    NEthos::TBinaryLabelFloatFeatureVector nextPairwiseInstance(currentPool.size(), std::move(featuresVector), label, weight);
                    currentPool.push_back(nextPairwiseInstance);
                }
            }
        }

        NEthos::TBinaryLabelLogisticRegressionLearner learner(runOptions.LinearClassifierOptions);
        learner.UpdateWeights(currentPool.begin(), currentPool.end(), weights);

        Cout << "epoch #" << (epoch + 1) << ":\n";
        NSc::TValue metrics = Metrics(currentPool, weights, runOptions).ToTValue();
        if (doPrintPairwiseMetrics) {
            metrics["BRPF"] = BadRankedPairsFraction(pool, weights);
        }
        PrintMetrics(metrics, "BRPF");
        Cout << Endl;
    }

    NEthos::TBinaryClassificationLinearModel linearModel(weights, 0.);
    if (runOptions.ModelPath) {
        TFixedBufferFileOutput modelOut(runOptions.ModelPath);
        linearModel.Save(&modelOut);
    }

    return 0;
}

int DoTest(int argc, const char** argv) {
    TRunOptions runOptions;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        runOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    NEthos::TBinaryLabelFloatFeatureVectors pool = NEthos::TBinaryLabelFloatFeatureVectors::FromFeatures(runOptions.FeaturesPath);

    NEthos::TBinaryClassificationLinearModel model;
    if (runOptions.ModelPath) {
        TFileInput modelIn(runOptions.ModelPath);
        model.Load(&modelIn);
    }

    NEthos::TBcMetrics metrics = Metrics(pool, model, runOptions);

    PrintMetrics(metrics.ToTValue(), "F1");

    return 0;
}

int DoApply(int argc, const char** argv) {
    TRunOptions runOptions;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        runOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    NEthos::TBinaryClassificationLinearModel model;
    if (runOptions.ModelPath) {
        TFileInput modelIn(runOptions.ModelPath);
        model.Load(&modelIn);
    }

    {
        auto input = OpenInput(runOptions.FeaturesPath);
        TString featuresStr;
        while (input->ReadLine(featuresStr)) {
            TStringBuf featuresStrBuf(featuresStr);
            Cout << featuresStrBuf.NextTok('\t') << '\t';
            Cout << featuresStrBuf.NextTok('\t') << '\t';
            Cout << featuresStrBuf.NextTok('\t') << '\t';
            Cout << featuresStrBuf.NextTok('\t') << '\t';

            NEthos::TBinaryLabelFloatFeatureVector instance = NEthos::TBinaryLabelFloatFeatureVector::FromFeaturesString(featuresStr);
            Cout << model.Apply(instance, &runOptions.ApplyOptions).Prediction << "\n";
        }
    }

    return 0;
}

int DoPrintModel(int argc, const char** argv) {
    TString modelPath;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("model", "model path")
            .Required()
            .StoreResult(&modelPath);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    NEthos::TBinaryClassificationLinearModel model;
    {
        TFileInput modelIn(modelPath);
        model.Load(&modelIn);
    }

    const NEthos::TFeatureWeightMap<float>& weights = model.GetWeights().GetWeightMap();

    TVector<size_t> featureIds;
    for (auto&& bs : weights) {
        featureIds.push_back(bs.first);
    }
    Sort(featureIds.begin(), featureIds.end());

    for (const size_t featureIdx : featureIds) {
        const float* weight = weights.FindPtr(featureIdx);
        Cout << Sprintf("\t%" PRISZT " : %.10lf\n", featureIdx, *weight);
    }
    Cout << "intercept : " << Sprintf("%.10lf", model.GetThreshold()) << Endl;

    return 0;
}

double Logify(const double value) {
    return value > 0 ? log(value + 1.) : -log(1. - value);
}

int DoLogify(int argc, const char** argv) {
    Y_UNUSED(argc);
    Y_UNUSED(argv);

    TString dataStr;
    while (Cin.ReadLine(dataStr)) {
        TStringBuf dataStrBuf(dataStr);
        Cout << dataStrBuf.NextTok('\t') << '\t';
        Cout << dataStrBuf.NextTok('\t') << '\t';
        Cout << dataStrBuf.NextTok('\t') << '\t';
        Cout << dataStrBuf.NextTok('\t');
        while (dataStrBuf) {
            const double value = FromString<double>(dataStrBuf.NextTok('\t'));
            Cout << '\t' << Logify(value);
        }
        Cout << '\n';
    }
    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("learn", DoLearn, "learn logistic regression model");
    modChooser.AddMode("learn-pairwise", DoPairwiseLearn, "learn logistic regression model with pairwise loss function");
    modChooser.AddMode("test", DoTest, "test logistic regression model");
    modChooser.AddMode("apply", DoApply, "apply logistic regression model");
    modChooser.AddMode("print", DoPrintModel, "print model weights");
    modChooser.AddMode("logify", DoLogify, "logify pool");
    return modChooser.Run(argc, argv);
}
