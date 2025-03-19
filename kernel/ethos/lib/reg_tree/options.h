#pragma once

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/set.h>
#include <util/generic/ymath.h>
#include <util/generic/string.h>
#include <util/string/type.h>
#include <util/string/cast.h>
#include <util/stream/file.h>

namespace NRegTree {

enum ESplitterType {
    DeviationSplitter,
    SimpleLinearRegressionSplitter,
    BestSimpleLinearRegressionSplitter,
    PositionalSimpleLinearRegressionSplitter,
    LinearRegressionSplitter,
};

enum ESolverType {
    ConstantSolver,
    BestSimpleLinearRegressionSplitterSolver,
    PositionalSimpleLinearRegressionSplitterSolver,
    LinearRegressionSolver
};

enum ETransformationType {
    Identity,
    SimpleLogify,
    SmartLogify,
    SimpleSigma,
    SmartSigma,
    Quantile,
};

template <typename T>
class TDependentValue {
public:
    T Value;
    bool ChoosedByUser;

    TDependentValue(T value = T(), bool choosed = false)
        : Value(value)
        , ChoosedByUser(choosed)
    {
    }

    void SetDefaultIfNotChoosed(T value) {
        if (!ChoosedByUser) {
            Value = value;
        }
    }
};

struct TOptions {
    size_t BoostingIterationsCount;
    size_t MaxDepth;
    TDependentValue<size_t> MinLeafSize;

    bool DoShrinkageOptimization;
    TDependentValue<double> Shrinkage;

    ESplitterType SplitterType;
    size_t SplitterIterationsCount;

    ESolverType SolverType;

    double RegularizationThreshold;
    double RegularizationParameter;

    size_t ThreadsCount;
    size_t SelectFeaturesThreads;

    TString LossFunctionName;

    size_t CVFoldsCount;
    size_t CVSeed;
    size_t CVRuns;

    bool OutputDetailedTreeInfo;

    double ClassificationThreshold;
    double LogisticOffset;

    TString PositiveMark;

    bool LastSplitOnly;

    bool UseWeights;

    size_t TuneThreads;

    TSet<size_t> IgnoredFeatures;

    bool WeightByQueries;

    bool UseQueries;

    double Bias;

    double PositivesFactor;

    size_t PairwiseOverhead;

    double PruneFactor;

    ETransformationType TransformationType;
    size_t TransformationIterations;

    size_t DiscretizationLevelsCount;

    bool OptimizeBinaryRankLoss;

    bool UnweightedMetrics;

    bool FlatPoolWeights;

    bool ExportMetricsInJson;
    TString JsonMetricsOutPath;

    bool AlternatePools;

    size_t IntentWeightStepsCount;
    double IntentWeightStep;
    double StartIntentWeight;
    bool DeflateIntentWeight;

    TString SurplusIWChooserType;
    double SurplusThreshold;

    bool DetailedSurplus;

    double QueryRegularizationFactor;

    bool ExportOnlyPrefictions;

    bool SelectBestThreshold;
    double FBeta;

    double MinPrecision;
    double MinRecall;

    TOptions()
        : BoostingIterationsCount(30)
        , MaxDepth(5)
        , MinLeafSize(1000)
        , DoShrinkageOptimization(true)
        , Shrinkage(0.1)
        , SplitterType(DeviationSplitter)
        , SplitterIterationsCount(10)
        , SolverType(BestSimpleLinearRegressionSplitterSolver)
        , RegularizationThreshold(1e-5)
        , RegularizationParameter(1e-5)
        , ThreadsCount(10)
        , SelectFeaturesThreads(2)
        , LossFunctionName("QUAD")
        , CVFoldsCount(5)
        , CVSeed(0)
        , CVRuns(1)
        , OutputDetailedTreeInfo(true)
        , ClassificationThreshold(0.)
        , LogisticOffset(0.)
        , LastSplitOnly(false)
        , UseWeights(false)
        , TuneThreads(5)
        , WeightByQueries(false)
        , UseQueries(false)
        , Bias(0.)
        , PositivesFactor(1.)
        , PairwiseOverhead(10)
        , PruneFactor(1.)
        , TransformationType(Identity)
        , TransformationIterations(10)
        , DiscretizationLevelsCount(16)
        , OptimizeBinaryRankLoss(false)
        , UnweightedMetrics(false)
        , FlatPoolWeights(false)
        , ExportMetricsInJson(false)
        , AlternatePools(false)
        , IntentWeightStepsCount(20)
        , IntentWeightStep(0.01)
        , StartIntentWeight(0.)
        , DeflateIntentWeight(false)
        , SurplusIWChooserType("max_positive_prediction")
        , SurplusThreshold(0.)
        , DetailedSurplus(false)
        , QueryRegularizationFactor(1.)
        , ExportOnlyPrefictions(false)
        , SelectBestThreshold(false)
        , FBeta(1.)
        , MinPrecision(-1.)
        , MinRecall(-1.)
    {
    }

    void AddOpts(NLastGetopt::TOpts& opts, bool extendedOptionNames = false) {
        if (extendedOptionNames) {
            opts.AddLongOption("reg-tree-iterations", "boosting iterations count")
                .StoreResult(&BoostingIterationsCount).DefaultValue(ToString(BoostingIterationsCount));
            opts.AddLongOption("reg-tree-shrinkage", "shrinkage")
                .StoreResult(&Shrinkage, Shrinkage);
            opts.AddLongOption("reg-tree-positives-factor", "positives weight factor")
                .StoreResult(&PositivesFactor).DefaultValue(ToString(PositivesFactor));
        } else {
            opts.AddCharOption('i', "boosting iterations count")
                .StoreResult(&BoostingIterationsCount).DefaultValue(ToString(BoostingIterationsCount));
            opts.AddCharOption('w', "shrinkage")
                .StoreResult(&Shrinkage, Shrinkage);
            opts.AddLongOption("positives-factor", "positives weight factor")
                .StoreResult(&PositivesFactor).DefaultValue(ToString(PositivesFactor));
        }

        opts.AddCharOption('n', "max tree depth")
            .StoreResult(&MaxDepth).DefaultValue(ToString(MaxDepth));
        opts.AddCharOption('s', "min leaf size")
            .StoreResult(&MinLeafSize);

        opts.AddLongOption("no-shrinkage-optimization", "shrinkage")
            .NoArgument().StoreValue(&DoShrinkageOptimization, false);
        opts.AddCharOption('W', "use 4th column as sample weight")
            .NoArgument().StoreValue(&UseWeights, true);

        opts.AddLongOption('T', "threads", "threads count")
            .StoreResult(&ThreadsCount).DefaultValue(ToString(ThreadsCount));
        opts.AddLongOption("sfthreads", "threads count for selecting features")
            .StoreResult(&SelectFeaturesThreads).DefaultValue(ToString(SelectFeaturesThreads));

        opts.AddLongOption("deviation-splits", "splitting with min-deviation splitter, is default")
            .NoArgument().StoreValue(&SplitterType, DeviationSplitter);
        opts.AddLongOption("slr-splits", "splitting with min simple linear regression loss splitter")
            .NoArgument().StoreValue(&SplitterType, SimpleLinearRegressionSplitter);
        opts.AddLongOption("bslr-splits", "splitting with min best simple linear regression loss splitter")
            .NoArgument().StoreValue(&SplitterType, BestSimpleLinearRegressionSplitter);
        opts.AddLongOption("plr-splits", "splitting with positional linear regression splitter")
            .NoArgument().StoreValue(&SplitterType, PositionalSimpleLinearRegressionSplitter);
        opts.AddLongOption("lr-splits", "splitting with linear regression splitter")
            .NoArgument().StoreValue(&SplitterType, LinearRegressionSplitter);

        opts.AddLongOption("splitter-iterations", "iterations count for iterative splitters and solvers")
            .StoreResult(&SplitterIterationsCount).DefaultValue(ToString(SplitterIterationsCount));

        opts.AddLongOption("folds", "folds count for cross-validation")
            .StoreResult(&CVFoldsCount).DefaultValue(ToString(CVFoldsCount));
        opts.AddLongOption("seed", "seed for cross-validation")
            .StoreResult(&CVSeed).DefaultValue(ToString(CVSeed));
        opts.AddLongOption("runs", "number of cross-validation runs")
            .StoreResult(&CVRuns).StoreValue(&OutputDetailedTreeInfo, false);

        opts.AddLongOption("logistic", "learn binary classificator with log-loss")
            .NoArgument().StoreValue(&LossFunctionName, "LOGISTIC");
        opts.AddLongOption("query-regularized-logistic", "by-query regularized logistic loss function")
            .NoArgument().StoreValue(&LossFunctionName, "QUERY_REGULARIZED_LOGISTIC").StoreValue(&AlternatePools, true);
        opts.AddLongOption("pairwise", "learn pairwise binary classificator")
            .NoArgument().StoreValue(&LossFunctionName, "PAIRWISE");
        opts.AddLongOption("rank", "learn ranking")
            .NoArgument().StoreValue(&LossFunctionName, "RANK");
        opts.AddLongOption("rank-binary", "learn ranking using binary ranking loss function for coefficient choosing")
            .NoArgument().StoreValue(&LossFunctionName, "RANK").StoreValue(&OptimizeBinaryRankLoss, true);
        opts.AddLongOption("surplus", "learn surplus (test mode)")
            .NoArgument().StoreValue(&LossFunctionName, "SURPLUS");
        opts.AddLongOption("surplus-new", "learn new surplus (test mode)")
            .NoArgument().StoreValue(&LossFunctionName, "NEW_SURPLUS");

        opts.AddCharOption('C', "classification threshold")
            .StoreResult(&ClassificationThreshold).DefaultValue(ToString(ClassificationThreshold));
        opts.AddLongOption("bias", "bias for untransformed predictions")
            .StoreResult(&Bias).DefaultValue(ToString(Bias));
        opts.AddLongOption("offset", "offset for logloss")
            .StoreResult(&LogisticOffset).DefaultValue(ToString(LogisticOffset));

        opts.AddLongOption("mark", "positive mark")
            .StoreResult(&PositiveMark);

        opts.AddLongOption("last-split", "use deviation-based splits on every leven except the last one")
            .NoArgument().StoreValue(&LastSplitOnly, true);

        opts.AddLongOption("tune-threads", "threads count for tuning")
            .StoreResult(&TuneThreads).DefaultValue(ToString(TuneThreads));

        opts.AddLongOption("constant-solver", "constant models in leafs")
            .NoArgument().StoreValue(&SolverType, ConstantSolver);
        opts.AddLongOption("bslr-solver", "best simple linear regression models in leafs")
            .NoArgument().StoreValue(&SolverType, BestSimpleLinearRegressionSplitterSolver);
        opts.AddLongOption("plr-solver", "position linear regression models in leafs")
            .NoArgument().StoreValue(&SolverType, PositionalSimpleLinearRegressionSplitterSolver);
        opts.AddLongOption("lr-solver", "linear regression models in leafs")
            .NoArgument().StoreValue(&SolverType, LinearRegressionSolver);

        opts.AddLongOption('I', "ignore", "ignore features (n1:n2-n3:n4...")
            .Handler1(std::bind(&TOptions::AddIgnoredFeatures, this, std::placeholders::_1));
        opts.AddLongOption("use", "use features (n1:n2-n3:n4...")
            .Handler1(std::bind(&TOptions::AddUsedFeatures, this, std::placeholders::_1));

        opts.AddLongOption("query-weights", "weight instances by queries")
            .NoArgument().StoreValue(&WeightByQueries, true);
        opts.AddLongOption("unweighted-metrics", "ignore weights in precision, recall and F1")
            .NoArgument().StoreValue(&UnweightedMetrics, true);

        opts.AddLongOption("pairwise-overhead", "random elements taking for every instance to produce interpolated AUC score")
            .StoreResult(&PairwiseOverhead).DefaultValue(ToString(PairwiseOverhead));

        opts.AddLongOption("prune", "prune factor")
            .StoreResult(&PruneFactor).DefaultValue(ToString(PruneFactor));

        opts.AddLongOption("logify", "simple logify features transformation")
            .NoArgument().StoreValue(&TransformationType, SimpleLogify);
        opts.AddLongOption("smart-logify", "smart logify features transformation")
            .NoArgument().StoreValue(&TransformationType, SmartLogify);
        opts.AddLongOption("sigma", "simple sigma features transformation")
            .NoArgument().StoreValue(&TransformationType, SimpleSigma);
        opts.AddLongOption("smart-sigma", "smart sigma features transformation")
            .NoArgument().StoreValue(&TransformationType, SmartSigma);
        opts.AddLongOption("quantile", "quantile features transformation")
            .NoArgument().StoreValue(&TransformationType, Quantile);
        opts.AddLongOption("transformation-iterations", "iterations for finding feature transformation")
            .StoreResult(&TransformationIterations).DefaultValue(ToString(TransformationIterations));
        opts.AddCharOption('x', "discretization levels count")
            .StoreResult(&DiscretizationLevelsCount).DefaultValue(ToString(DiscretizationLevelsCount));

        opts.AddLongOption("flat-pool-weights", "pool weight means sum of its elements (both queries and elements)")
            .NoArgument().StoreValue(&FlatPoolWeights, true);

        opts.AddLongOption("json-metrics", "export metrics in json (for --test)")
            .NoArgument().StoreValue(&ExportMetricsInJson, true);
        opts.AddLongOption("json-metrics-out", "export json metrics to file")
            .StoreResult(&JsonMetricsOutPath);

        opts.AddLongOption("alternate-pools", "alternate learning pools")
            .NoArgument().StoreValue(&AlternatePools, true);

        opts.AddLongOption("iw-steps-count", "intent weihgt steps count")
            .StoreResult(&IntentWeightStepsCount);
        opts.AddLongOption("iw-step", "intent weight step")
            .StoreResult(&IntentWeightStep);
        opts.AddLongOption("iw-start", "start intent weight")
            .StoreResult(&StartIntentWeight);
        opts.AddLongOption("iw-deflate", "deflate intent weight")
            .NoArgument()
            .StoreValue(&DeflateIntentWeight, true);
        opts.AddLongOption("random-props", "random properties json file name")
            .Handler1(std::bind(&TOptions::ProcessRandomProps, this, std::placeholders::_1));

        opts.AddLongOption("iw-chooser-type", "intent weight chooser type")
            .StoreResult(&SurplusIWChooserType);
        opts.AddLongOption("surplus-threshold", "threshold for surplus metrics")
            .StoreResult(&SurplusThreshold);

        opts.AddLongOption("detailed-surplus", "export detailed surplus info")
            .NoArgument()
            .StoreValue(&DetailedSurplus, true);

        opts.AddLongOption("query-regularization", "query regularization factor")
            .StoreResult(&QueryRegularizationFactor);

        opts.AddLongOption("export-predictions-only", "export only predictions in --apply mode")
            .NoArgument()
            .StoreValue(&ExportOnlyPrefictions, true);

        if (extendedOptionNames) {
            opts.AddLongOption("reg-tree-select-best-threshold", "select best threshold for classification")
                .NoArgument()
                .StoreValue(&SelectBestThreshold, true);
        } else {
            opts.AddLongOption("select-best-threshold", "select best threshold for classification")
                .NoArgument()
                .StoreValue(&SelectBestThreshold, true);
        }

        opts.AddLongOption("f-beta", "a coefficient for optimizing f measure with threshold")
            .StoreResult(&FBeta);
        opts.AddLongOption("min-recall", "minimal allowed recall")
            .StoreResult(&MinRecall);
        opts.AddLongOption("min-precision", "minimal allowed precision")
            .StoreResult(&MinPrecision);
    }

    void SetupDependentValues(size_t poolSize, size_t featuresCount) {
        MinLeafSize.SetDefaultIfNotChoosed(Max(std::ceil(poolSize * 0.001), featuresCount * 2.));
        Shrinkage.SetDefaultIfNotChoosed(2. / (std::log((double) BoostingIterationsCount) + 2.));
    }
private:
    void ProcessRandomProps(const NLastGetopt::TOptsParser* p) {
        const TString randomPropsFileName(p->CurVal());

        TFileInput randomPropsIn(randomPropsFileName);
        const NSc::TValue randomProps = NSc::TValue::FromJson(randomPropsIn.ReadAll());

        IntentWeightStepsCount = randomProps["steps_count"].GetNumber();
        IntentWeightStep = randomProps["intent_weight_step"].GetNumber();
        StartIntentWeight = randomProps["start_intent_weight"].GetNumber();
    }

    void AddIgnoredFeatures(const NLastGetopt::TOptsParser* p) {
        TStringBuf valuesBuf(p->CurVal());

        while (!!valuesBuf) {
            TStringBuf numberBuf = valuesBuf.NextTok(':');
            TStringBuf numberBufBackup = numberBuf;

            if (IsNumber(numberBuf)) {
                IgnoredFeatures.insert(FromString<size_t>(numberBuf));
                continue;
            }

            TStringBuf firstNumberBuf = numberBuf.NextTok('-');
            if (!IsNumber(firstNumberBuf) || !IsNumber(numberBuf)) {
                ythrow yexception() << "error parsing interval \"" << numberBufBackup << "\"";
            }

            size_t firstNumber = FromString<size_t>(firstNumberBuf);
            size_t lastNumber = FromString<size_t>(numberBuf);

            for (size_t featureNumber = firstNumber; featureNumber <= lastNumber; ++featureNumber) {
                IgnoredFeatures.insert(featureNumber);
            }
        }
    }

    void AddUsedFeatures(const NLastGetopt::TOptsParser* p) {
        for (size_t i = 0; i < 10000; ++i) {
            IgnoredFeatures.insert(i);
        }

        TStringBuf valuesBuf(p->CurVal());

        while (!!valuesBuf) {
            TStringBuf numberBuf = valuesBuf.NextTok(':');
            TStringBuf numberBufBackup = numberBuf;

            if (IsNumber(numberBuf)) {
                IgnoredFeatures.erase(FromString<size_t>(numberBuf));
                continue;
            }

            TStringBuf firstNumberBuf = numberBuf.NextTok('-');
            if (!IsNumber(firstNumberBuf) || !IsNumber(numberBuf)) {
                ythrow yexception() << "error parsing interval \"" << numberBufBackup << "\"";
            }

            size_t firstNumber = FromString<size_t>(firstNumberBuf);
            size_t lastNumber = FromString<size_t>(numberBuf);

            for (size_t featureNumber = firstNumber; featureNumber <= lastNumber; ++featureNumber) {
                IgnoredFeatures.erase(featureNumber);
            }
        }
    }
};

}

template<>
class THash<NRegTree::TOptions> {
public:
    size_t operator () (const NRegTree::TOptions& options) const {
        THash<size_t> hasher;
        return CombineHashes(hasher(options.MaxDepth),
               CombineHashes(hasher(options.MinLeafSize.Value),
                             THash<double>()(options.Shrinkage.Value)));
    }
};

template <>
inline NRegTree::TDependentValue<size_t> FromString(const TStringBuf& s) {
    return NRegTree::TDependentValue<size_t>(FromString<size_t>(s), true);
}

template <>
inline NRegTree::TDependentValue<double> FromString(const TStringBuf& s) {
    return NRegTree::TDependentValue<double>(FromString<double>(s), true);
}
