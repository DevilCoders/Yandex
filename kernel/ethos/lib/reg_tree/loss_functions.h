#pragma once

#include "least_squares_tree.h"
#include "pool.h"
#include "splitter.h"
#include "optimization.h"

#include <library/cpp/scheme/scheme.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/ymath.h>
#include <util/string/printf.h>
#include <util/system/mutex.h>

namespace NRegTree {

struct TMetricWithValues {
    TString MetricName;

    double LearnValue;
    double TestValue;

    TMetricWithValues(const TString& name, double learnValue = 0., double testValue = 0.)
        : MetricName(name)
        , LearnValue(learnValue)
        , TestValue(testValue)
    {
    }

    const TMetricWithValues& operator += (const TMetricWithValues& other) {
        Y_ASSERT(MetricName == other.MetricName);

        LearnValue += other.LearnValue;
        TestValue += other.TestValue;

        return *this;
    }

    TString ToString(size_t normalizer = 1) const {
        TString result = MetricName;
        for (size_t len = result.length(); len < 10; ++len) {
            result.push_back(' ');
        }

        double avgLearnValue = LearnValue / normalizer;
        if (fabs(avgLearnValue) > 1. && floor(avgLearnValue) == avgLearnValue) {
            result += "learn: " + Sprintf("%.0lf", avgLearnValue);
        } else {
            result += "learn: " + Sprintf("%.6lf", avgLearnValue);
        }

        if (!TestValue) {
            return result;
        }

        for (size_t len = result.length(); len < 29; ++len) {
            result.push_back(' ');
        }

        double avgTestValue = TestValue / normalizer;
        if (fabs(avgTestValue) > 1. && floor(avgTestValue) == avgTestValue) {
            result += "test: " + Sprintf("%.0lf", avgTestValue);
        } else {
            result += "test: " + Sprintf("%.6lf", avgTestValue);
        }

        return result;
    }
};

inline void PrintMetrics(IOutputStream& out, const TVector<TMetricWithValues>& metrics, size_t normalizer = 1) {
    for (const TMetricWithValues* metric = metrics.begin(); metric != metrics.end(); ++metric) {
        out << "    " << metric->ToString(normalizer) << "\n";
    }
}

inline NSc::TValue MetricsToJson(const TVector<TMetricWithValues>& metrics,
                                 bool exportLearnMetrics = true,
                                 size_t normalizer = 1)
{
    NSc::TValue metricsValue;
    for (const TMetricWithValues& metric : metrics) {
        metricsValue[metric.MetricName] = (exportLearnMetrics ? metric.LearnValue : metric.TestValue) / normalizer;
    }
    return metricsValue;
}

inline void PrintMetrics(IOutputStream& out, const TVector<TVector<TMetricWithValues> >& poolMetrics, size_t normalizer = 1) {
    if (poolMetrics.size() == 1) {
        PrintMetrics(out, poolMetrics[0], normalizer);
        return;
    }

    for (size_t poolNumber = 0; poolNumber < poolMetrics.size(); ++poolNumber) {
        out << "metrics for pool #" << poolNumber << ":\n";
        PrintMetrics(out, poolMetrics[poolNumber], normalizer);
        out << "\n";
    }
}

inline NSc::TValue MetricsToJson(const TVector<TVector<TMetricWithValues> >& poolMetrics,
                                 const TVector<NRegTree::TPoolFileNameInfo>& learnPoolFileNameInfos,
                                 const TString& testPoolFileName,
                                 size_t normalizer = 1)
{
    Y_ASSERT(poolMetrics.size() == learnPoolFileNameInfos.size());

    NSc::TValue learnMetricsValue;
    NSc::TValue testMetricsValue;
    for (size_t poolNumber = 0; poolNumber < poolMetrics.size(); ++poolNumber) {
        NSc::TValue learnPoolValue = MetricsToJson(poolMetrics[poolNumber], true, normalizer);
        learnPoolValue["pool_weight"] = learnPoolFileNameInfos[poolNumber].PoolWeight;
        learnMetricsValue[learnPoolFileNameInfos[poolNumber].FeaturesFileName] = learnPoolValue;

        if (!!testPoolFileName && poolNumber == 0) {
            NSc::TValue testPoolValue = MetricsToJson(poolMetrics[poolNumber], false, normalizer);
            testMetricsValue[testPoolFileName] = testPoolValue;
        }
    }
    NSc::TValue metricsValue;
    metricsValue["learn"] = learnMetricsValue;
    metricsValue["test"] = testMetricsValue;
    return metricsValue;
}

inline void PrintMetricsJson(IOutputStream& out,
                             const TVector<TVector<TMetricWithValues> >& poolMetrics,
                             const TVector<NRegTree::TPoolFileNameInfo>& poolFileNameInfos,
                             const TString& testPoolFileName,
                             size_t normalizer = 1)
{
    NSc::TValue metricsValue = MetricsToJson(poolMetrics, poolFileNameInfos, testPoolFileName, normalizer);
    out << metricsValue.ToJson() << "\n";
}

inline void PrintMetrics(IOutputStream& out,
                         const TVector<TVector<TMetricWithValues> >& poolMetrics,
                         const TVector<NRegTree::TPoolFileNameInfo>& poolFileNameInfos,
                         const TString& testPoolFileName,
                         const TOptions& options,
                         size_t normalizer = 1)
{
    if (options.ExportMetricsInJson) {
        PrintMetricsJson(out, poolMetrics, poolFileNameInfos, testPoolFileName, normalizer);
    } else {
        PrintMetrics(out, poolMetrics, normalizer);
    }
}

template <typename TFloatType>
inline TFloatType Sigma(TFloatType value) {
    if (value < TFloatType(-15)) {
        return TFloatType();
    } else if (value > TFloatType(15)) {
        return TFloatType(1);
    }

    return TFloatType(1) / (TFloatType(1) + exp(-value));
}

template <typename TFloatType>
inline TFloatType LogError(TFloatType value) {
    if (value < TFloatType(-15)) {
        return -value;
    } else if (value > TFloatType(15)) {
        return TFloatType();
    }

    return log(TFloatType(1) + exp(-value));
}

namespace NPrivate {

class TFastSigmaCalculator {
private:
    double Min;
    double Max;

    double Step;

    TVector<std::pair<double, double> > LinearModels;
public:
    TFastSigmaCalculator(double min = -15, double max = 15, size_t count = 51)
        : Min(min)
        , Max(max)
        , Step((Max - Min) / count)
        , LinearModels(count)
    {
        TVector<TSimpleLinearRegressionSplitterBase<double> > simpleLinearRegressions(count);
        for (double x = min; x <= max; x += Step / count / 50) {
            size_t number = (x - min) / Step;
            simpleLinearRegressions[number].Add(Sigma(x), 1., x);
        }

        for (size_t i = 0; i < count; ++i) {
            simpleLinearRegressions[i].Solve(LinearModels[i].first, LinearModels[i].second);
        }
    }

    double CalculateSigma(double arg) {
        return arg > Max ? 1.
             : arg < Min ? 0.
             : CalculateLinearPrediction(arg);
    }
private:
    inline double CalculateLinearPrediction(double arg) const {
        size_t number = (arg - Min) / Step;
        const std::pair<double, double>& simpleLinearRegressionModel = LinearModels[number];
        return simpleLinearRegressionModel.first * arg + simpleLinearRegressionModel.second;
    }
};

class TFastLogErrorCalculator {
private:
    double Min;
    double Max;

    double Step;

    TVector<std::pair<double, double> > LinearModels;
public:
    TFastLogErrorCalculator(double min = -15, double max = 15, size_t count = 51)
        : Min(min)
        , Max(max)
        , Step((Max - Min) / count)
        , LinearModels(count)
    {
        TVector<TSimpleLinearRegressionSplitterBase<double> > simpleLinearRegressions(count);
        for (double x = min; x <= max; x += Step / count / 50) {
            size_t number = (x - min) / Step;
            simpleLinearRegressions[number].Add(LogError(x), 1., x);
        }

        for (size_t i = 0; i < count; ++i) {
            simpleLinearRegressions[i].Solve(LinearModels[i].first, LinearModels[i].second);
        }
    }

    inline double CalculateLogError(double arg) const {
        return arg > Max ? 0.
             : arg < Min ? -arg
             : CalculateLinearPrediction(arg);
    }
private:
    inline double CalculateLinearPrediction(double arg) const {
        size_t number = (arg - Min) / Step;
        const std::pair<double, double>& simpleLinearRegressionModel = LinearModels[number];
        return simpleLinearRegressionModel.first * arg + simpleLinearRegressionModel.second;
    }
};

} // namespace NPrivate

template <typename TFloatType>
inline TFloatType FastSigma(TFloatType value) {
    return Singleton<NPrivate::TFastSigmaCalculator>()->CalculateSigma(value);
}

template <typename TFloatType>
inline TFloatType FastLogError(TFloatType value) {
    return Singleton<NPrivate::TFastLogErrorCalculator>()->CalculateLogError(value);
}

class IIntentWeightChooser {
public:
    virtual ~IIntentWeightChooser() {
    }

    virtual void Add(const double iw, const double prediction) = 0;
    virtual TMaybe<double> GetBestIW() const = 0;
};

class TMaxPositivePredictionIWChooser : public IIntentWeightChooser {
private:
    double BestIW;
    double MaxPrediction;
    bool Setup = false;
public:
    TMaxPositivePredictionIWChooser(const double threshold = 0., const double startIntentWeight = 0.)
        : BestIW(startIntentWeight)
        , MaxPrediction(threshold)
    {
    }

    void Add(const double iw, const double prediction) override {
        if (prediction <= MaxPrediction) {
            return;
        }

        Setup = true;
        BestIW = iw;
        MaxPrediction = prediction;
    }

    TMaybe<double> GetBestIW() const override {
        if (!Setup) {
            return Nothing();
        }
        return BestIW;
    }
};

class TMaxIWWithPositivePredictionChooser : public IIntentWeightChooser {
private:
    double BestIW;
    const double Threshold;
    bool Setup = false;
public:
    TMaxIWWithPositivePredictionChooser(const double threshold, const double startIntentWeight = 0.)
        : BestIW(startIntentWeight)
        , Threshold(threshold)
    {
    }

    void Add(const double iw, const double prediction) override {
        if (prediction <= Threshold) {
            return;
        }

        Setup = true;
        BestIW = iw;
    }

    TMaybe<double> GetBestIW() const override {
        if (!Setup) {
            return Nothing();
        }
        return BestIW;
    }
};

class TMaxPredictionIWChooser : public IIntentWeightChooser {
private:
    double BestIW;
    double Threshold;

    bool Setup = false;
public:
    TMaxPredictionIWChooser(const double startIntentWeight = 0.)
        : BestIW(startIntentWeight)
        , Threshold(-1e25)
    {
    }

    void Add(const double iw, const double prediction) override {
        if (Setup && prediction <= Threshold) {
            return;
        }

        Setup = true;
        BestIW = iw;
        Threshold = prediction;
    }

    TMaybe<double> GetBestIW() const override {
        if (!Setup) {
            return Nothing();
        }
        return BestIW;
    }
};

class TMaxProfitIWChooser : public IIntentWeightChooser {
private:
    double BestIW;
    double MaxProfit = 0.;
    const double Threshold;
    bool Setup = false;
public:
    TMaxProfitIWChooser(const double threshold, const double startIntentWeight = 0.)
        : BestIW(startIntentWeight)
        , Threshold(threshold)
    {
    }

    void Add(const double iw, const double prediction) override {
        if (prediction <= Threshold) {
            return;
        }

        const double profit = iw * FastSigma(prediction);
        if (profit <= MaxProfit) {
            return;
        }

        Setup = true;
        BestIW = iw;
        MaxProfit = profit;
    }

    TMaybe<double> GetBestIW() const override {
        if (!Setup) {
            return Nothing();
        }
        return BestIW;
    }
};

static inline THolder<IIntentWeightChooser> CreateIntentWeightChooser(const TStringBuf chooserType, const double threshold, const double startIntentWeight) {
    if (chooserType == "max_iw_with_positive_prediction") {
        return MakeHolder<TMaxIWWithPositivePredictionChooser>(threshold, startIntentWeight);
    }
    if (chooserType == "max_profit") {
        return MakeHolder<TMaxProfitIWChooser>(threshold, startIntentWeight);
    }
    if (chooserType == "max_positive_prediction") {
        return MakeHolder<TMaxPositivePredictionIWChooser>(threshold, startIntentWeight);
    }
    if (chooserType == "max_prediction") {
        return MakeHolder<TMaxPredictionIWChooser>(startIntentWeight);
    }
    return MakeHolder<TMaxPositivePredictionIWChooser>(threshold, startIntentWeight);
}

template <typename TFloatType>
class TLossFunctionBase {
private:
public:
    virtual ~TLossFunctionBase() {
    }

    virtual TString Name() const = 0;

    virtual void InitializeGoals(TPool<TFloatType>& learnPool, const TOptions& options) = 0;

    // calculates metrics and modifies instance goals
    virtual TVector<TMetricWithValues> Process(TPool<TFloatType>& pool,
                                               TLeastSquaresTree<TFloatType>& newTree,
                                               const TOptions& options,
                                               const THashSet<size_t>& testInstanceNumbers,
                                               TPool<TFloatType>* testPool,
                                               const size_t poolToUseNumber,
                                               const bool treeIsFirst,
                                               TFloatType* bias = nullptr) = 0;

    // calculates metrics only
    virtual TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool,
                                                  const TOptions& options,
                                                  size_t poolNumber = 0) = 0;

    TVector<TVector<TMetricWithValues> > ProcessAllPools(TPool<TFloatType>& pool,
                                                         TLeastSquaresTree<TFloatType>& newTree,
                                                         const TOptions& options,
                                                         const THashSet<size_t>& testInstanceNumbers,
                                                         TPool<TFloatType>* testPool,
                                                         const size_t poolToUseNumber,
                                                         const bool treeIsFirst,
                                                         TFloatType* bias = nullptr)
    {
        TVector<TVector<TMetricWithValues> > result;
        result.push_back(Process(pool, newTree, options, testInstanceNumbers, testPool, poolToUseNumber, treeIsFirst, bias));

        for (size_t poolNumber = 1; poolNumber < pool.GetPoolsCount(); ++poolNumber) {
            result.push_back(GetMetrics(pool, options, poolNumber));
        }

        return result;
    }

    virtual TFloatType TransformPrediction(TFloatType prediction) const = 0;
};

template <typename TFloatType>
class TRegressionMetricsCalculator {
private:
    TFloatType SumGoals;
    TFloatType SumSquaredGoals;

    TFloatType SumSquaredErrors;

    TFloatType SumWeights;
public:
    TRegressionMetricsCalculator()
        : SumGoals(TFloatType())
        , SumSquaredGoals(TFloatType())
        , SumSquaredErrors(TFloatType())
        , SumWeights(TFloatType())
    {
    }

    void Add(TFloatType goal, TFloatType prediction, TFloatType weight) {
        TFloatType weightedGoal = goal * weight;

        SumGoals += weightedGoal;
        SumSquaredGoals += weightedGoal * goal;

        TFloatType error = goal - prediction;
        SumSquaredErrors += error * error * weight;

        SumWeights += weight;
    }

    void Add(const TInstance<TFloatType>& instance) {
        Add(instance.OriginalGoal, instance.Prediction, instance.Weight);
    }

    TFloatType RootMeanSquaredError() const {
        return SumWeights ? sqrt(Max(TFloatType(), SumSquaredErrors) / SumWeights) : TFloatType();
    }

    TFloatType DeterminationCoefficient() const {
        if (!SumWeights) {
            return TFloatType();
        }

        TFloatType sumSquaredGoalDeviations = SumSquaredGoals - SumGoals / SumWeights * SumGoals;
        if (!sumSquaredGoalDeviations) {
            return TFloatType();
        }

        return 1 - SumSquaredErrors / sumSquaredGoalDeviations;
    }
};

template <typename TFloatType>
class TSimpleClassificationMetricsCalculator {
private:
    TFloatType SumLogisticLosses;

    TFloatType SumWeights;

    TFloatType TruePositives;
    TFloatType FalsePositives;

    TFloatType ActualPositives;

    TFloatType RightAnswers;

    struct TPredictionData {
        double Prediction;
        double Weight;
        bool Result;

        TPredictionData(double prediction = 0, double weight = 0, bool result = false)
            : Prediction(prediction)
            , Weight(weight)
            , Result(result)
        {
        }

        bool operator < (const TPredictionData& other) const {
            if (Prediction == other.Prediction) {
                return Result && !other.Result;
            }
            return Prediction < other.Prediction;
        }

        bool operator > (const TPredictionData& other) const {
            return other < *this;
        }
    };

    TVector<TPredictionData> Predictions;
public:
    TSimpleClassificationMetricsCalculator()
        : SumLogisticLosses(TFloatType())
        , SumWeights(TFloatType())
        , TruePositives(TFloatType())
        , FalsePositives(TFloatType())
        , ActualPositives(TFloatType())
        , RightAnswers(TFloatType())
    {
    }

    void Add(TFloatType predictedValue, bool predictionIsPositive, bool goal, TFloatType weight, bool unweightedMetrics = false) {
        TFloatType margin = goal ? predictedValue : -predictedValue;
        SumLogisticLosses += LogError(margin) * weight;

        Predictions.push_back(TPredictionData(predictedValue, unweightedMetrics ? 1. : weight, goal));

        RightAnswers += predictionIsPositive == goal ? weight : TFloatType();

        TFloatType modifiedWeight = unweightedMetrics ? TFloatType(1) : weight;

        TruePositives += predictionIsPositive && goal ? modifiedWeight : TFloatType();
        FalsePositives += predictionIsPositive && !goal ? modifiedWeight : TFloatType();

        ActualPositives += goal ? modifiedWeight : TFloatType();

        SumWeights += weight;
    }

    void Add(const TInstance<TFloatType>& instance, const TOptions& options = TOptions(), const TFloatType* additionalBias = nullptr) {
        const double predictionWithBias = (additionalBias ? *additionalBias : 0.) + instance.Prediction + options.Bias;
        Add(predictionWithBias,
            predictionWithBias > TFloatType(),
            instance.OriginalGoal > options.ClassificationThreshold,
            instance.Weight,
            options.UnweightedMetrics);
    }

    bool Empty() const {
        return !SumWeights;
    }

    bool HasPositivePredictions() const {
        return TruePositives + FalsePositives > TFloatType();
    }

    bool HasPositiveGoals() const {
        return ActualPositives > TFloatType();
    }

    TFloatType LogisticLoss() const {
        return SumWeights ? SumLogisticLosses / SumWeights : TFloatType();
    }

    TFloatType Precision() const {
        return TruePositives ? (TFloatType) TruePositives / (TruePositives + FalsePositives) : TFloatType();
    }

    TFloatType Recall() const {
        return TruePositives ? (TFloatType) TruePositives / ActualPositives : TFloatType();
    }

    TFloatType F1() const {
        if (!TruePositives) {
            return TFloatType();
        }

        TFloatType precision = Precision();
        TFloatType recall = Recall();

        return 2 * precision * recall / (precision + recall);
    }

    TFloatType Accuracy() const {
        if (!RightAnswers) {
            return TFloatType();
        }

        return RightAnswers / SumWeights;
    }

    TFloatType AUC() const {
        TVector<TPredictionData> sdata(Predictions);
        Sort(sdata.begin(), sdata.end());

        size_t positivesCount = 0;
        size_t negativesCount = 0;

        double positivePositionsSum = 0;
        for (size_t i = 0; i < sdata.size(); ++i) {
            positivePositionsSum += sdata[i].Result ? i + 1 : 0;
            positivesCount += sdata[i].Result;
            negativesCount += !sdata[i].Result;
        }

        return (negativesCount && positivesCount)
             ? (positivePositionsSum - (double) positivesCount * (positivesCount + 1) / 2) / positivesCount / negativesCount
             : 0;
    }

    TFloatType BestFBeta(const TFloatType beta, const TFloatType minRecall, const TFloatType minPrecision, TFloatType* threshold) const {
        if (Predictions.empty() || !ActualPositives) {
            if (!!threshold) {
                *threshold = Max<TFloatType>();
            }
            return TFloatType();
        }

        TVector<TPredictionData> sortedPredictions(Predictions);
        Sort(sortedPredictions.begin(), sortedPredictions.end(), TGreater<>());

        TFloatType bestFBeta = (1. + beta) * ActualPositives / (SumWeights + beta * ActualPositives);
        if (!!threshold) {
            *threshold = Min<TFloatType>();
        }

        TFloatType truePositives = 0;
        TFloatType sumWeights = 0;
        for (size_t divisionPosition = 1; divisionPosition < sortedPredictions.size(); ++divisionPosition) {
            sumWeights += sortedPredictions[divisionPosition - 1].Weight;
            if (sortedPredictions[divisionPosition - 1].Result) {
                truePositives += sortedPredictions[divisionPosition - 1].Weight;
            }

            const TFloatType nextPrediction = sortedPredictions[divisionPosition].Prediction;
            const TFloatType previousPrediction = sortedPredictions[divisionPosition - 1].Prediction;

            if (!truePositives || nextPrediction + 1e-6 > previousPrediction) {
                continue;
            }

            const TFloatType precision = (TFloatType) truePositives / sumWeights;
            const TFloatType recall = (TFloatType) truePositives / ActualPositives;
            if (precision < minPrecision || recall < minRecall) {
                continue;
            }

            const TFloatType fBeta = (1. + beta) * truePositives / (sumWeights + beta * ActualPositives);
            if (threshold && fBeta > bestFBeta) {
                *threshold = (previousPrediction + nextPrediction) / 2;
            }
            bestFBeta = Max(fBeta, bestFBeta);
        }

        return bestFBeta;
    }
    TFloatType BestF1(TFloatType* threshold) const {
        return BestFBeta(1., -1., -1., threshold);
    }

    TFloatType BestF1() const {
        return BestF1(nullptr);
    }
};

template <typename TFloatType>
class TClassificationMetricsCalculator {
public:
    typedef TSimpleClassificationMetricsCalculator<TFloatType> TCalculator;
private:
    TVector<TCalculator> ByQueryCalculators;

    THashSet<size_t> UsedQueryIds;

    TSimpleClassificationMetricsCalculator<TFloatType> OverallCalculator;
public:
    TClassificationMetricsCalculator(size_t queriesCount)
        : ByQueryCalculators(queriesCount)
    {
    }

    void Add(const TInstance<TFloatType>& instance, const TOptions& options = TOptions(), const TFloatType* additionalBias = nullptr) {
        (options.UseQueries ? ByQueryCalculators[instance.QueryId] : OverallCalculator).Add(instance, options, additionalBias);
        UsedQueryIds.insert(instance.QueryId);
    }

    void Add(TFloatType predictedValue, bool predictionIsPositive, bool goal, TFloatType weight, size_t queryId, const TOptions& options = TOptions()) {
        (options.UseQueries ? ByQueryCalculators[queryId] : OverallCalculator).Add(predictedValue + options.Bias, predictionIsPositive, goal, weight, options.UnweightedMetrics);
        UsedQueryIds.insert(queryId);
    }

    TFloatType LogisticLoss() const {
        return GetMetric(&TCalculator::LogisticLoss);
    }

    TFloatType Precision() const {
        return GetMetric(&TCalculator::Precision, true);
    }

    TFloatType Recall() const {
        return GetMetric(&TCalculator::Recall, true);
    }

    TFloatType F1() const {
        return GetMetric(&TCalculator::F1, true);
    }

    TFloatType BestF1() const {
        return GetMetric(&TCalculator::BestF1, true);
    }

    const TCalculator& GetQueryCalculator(size_t queryIdx) const {
        return ByQueryCalculators[queryIdx];
    }

    TFloatType Accuracy() {
        return GetMetric(&TCalculator::Accuracy);
    }

    TFloatType AUC() {
        return GetMetric(&TCalculator::AUC);
    }
private:
    TFloatType GetMetric(TFloatType (TCalculator::*metric)() const, bool checkPositiveGoals = false) const {
        TFloatType result = TFloatType();
        size_t normalizer = 0;
        for (const TCalculator& calculator : ByQueryCalculators) {
            if (checkPositiveGoals && !calculator.HasPositiveGoals()) {
                continue;
            }
            if (calculator.Empty()) {
                continue;
            }
            result += (calculator.*metric)();
            ++normalizer;
        }
        if (normalizer) {
            return result / normalizer;
        }
        return (OverallCalculator.*metric)();
    }
};

template <typename TFloatType>
class TRankingMetricsCalculator {
private:
    struct TRankingPredictionData {
        double OriginalGoal;
        double Prediction;

        TRankingPredictionData(double originalGoal, double prediction)
            : OriginalGoal(originalGoal)
            , Prediction(prediction)
        {
        }
    };

    struct TQueryData {
        TVector<TRankingPredictionData> Predictions;
        double Weight;
    };

    TVector<TQueryData> Queries;
public:
    TRankingMetricsCalculator(size_t queriesCount)
        : Queries(queriesCount)
    {
    }

    void Add(size_t queryId, double originalGoal, double prediction, double queryWeight) {
        Queries[queryId].Predictions.push_back(TRankingPredictionData(originalGoal, prediction));
        Queries[queryId].Weight = queryWeight;
    }

    void Add(const TInstance<TFloatType>& instance, double queryWeight) {
        Add(instance.QueryId, instance.OriginalGoal, instance.Prediction, queryWeight);
    }

    TFloatType TopScore(size_t topSize) {
        TFloatType sumTopScores = TFloatType();
        TFloatType sumQueryWeights = TFloatType();

        for (TQueryData& query : Queries) {
            TFloatType topScore = TopScore(query.Predictions, &TRankingPredictionData::Prediction, topSize);
            TFloatType idealTopScore = TopScore(query.Predictions, &TRankingPredictionData::OriginalGoal, topSize);

            if (!idealTopScore) {
                continue;
            }

            sumTopScores += topScore / idealTopScore * query.Weight;
            sumQueryWeights += query.Weight;
        }

        return sumTopScores ? sumTopScores / sumQueryWeights : TFloatType();
    }

    TFloatType PFound() {
        TFloatType sumPfound = TFloatType();
        TFloatType sumQueryWeights = TFloatType();
        for (TQueryData& query : Queries) {
            if (query.Predictions.empty()) {
                continue;
            }

            sumPfound += PFound(query.Predictions, &TRankingPredictionData::Prediction) * query.Weight;
            sumQueryWeights += query.Weight;
        }
        return sumPfound ? sumPfound / sumQueryWeights : TFloatType();
    }

    TFloatType DCG() {
        TFloatType sumDCG = TFloatType();
        TFloatType sumQueryWeights = TFloatType();
        for (TQueryData& query : Queries) {
            if (query.Predictions.empty()) {
                continue;
            }

            sumDCG += DCG(query.Predictions, &TRankingPredictionData::Prediction) * query.Weight;
            sumQueryWeights += query.Weight;
        }
        return sumDCG ? sumDCG / sumQueryWeights : TFloatType();
    }

    TFloatType NPFound() {
        TFloatType sumNPfound = TFloatType();
        TFloatType sumQueryWeights = TFloatType();

        for (TQueryData& query : Queries) {
            TFloatType pfound = PFound(query.Predictions, &TRankingPredictionData::Prediction);
            TFloatType idealPfound = PFound(query.Predictions, &TRankingPredictionData::OriginalGoal);

            if (!idealPfound) {
                continue;
            }

            sumQueryWeights += query.Weight;
            sumNPfound += pfound ? pfound / idealPfound * query.Weight : TFloatType();
        }

        return sumNPfound ? sumNPfound / sumQueryWeights : TFloatType();
    }

    TFloatType NDCG() {
        TFloatType sumDCG = TFloatType();
        TFloatType sumQueryWeights = TFloatType();

        for (TQueryData& query : Queries) {
            TFloatType dcg = DCG(query.Predictions, &TRankingPredictionData::Prediction);
            TFloatType idealDcg = DCG(query.Predictions, &TRankingPredictionData::OriginalGoal);

            if (!idealDcg) {
                continue;
            }

            sumQueryWeights += query.Weight;
            sumDCG += dcg ? dcg / idealDcg * query.Weight : TFloatType();
        }

        return sumDCG ? sumDCG / sumQueryWeights : TFloatType();
    }
private:
    void SortQuery(TVector<TRankingPredictionData>& query, double TRankingPredictionData::* sortField, size_t limit = 0) {
        auto comparator = [sortField](const TRankingPredictionData& lhs, const TRankingPredictionData& rhs) {
            if (lhs.*sortField != rhs.*sortField) {
                return lhs.*sortField > rhs.*sortField;
            }
            return lhs.OriginalGoal < rhs.OriginalGoal;
        };

        if (limit && query.size() > limit) {
            PartialSort(query.begin(), query.begin() + limit, query.end(), comparator);
        } else {
            Sort(query.begin(), query.end(), comparator);
        }
    }

    TFloatType PFound(TVector<TRankingPredictionData>& query, double TRankingPredictionData::* sortField) {
        SortQuery(query, sortField);

        TFloatType pfound = TFloatType();
        TFloatType probability = 1;

        for (const TRankingPredictionData& rpd : query) {
            pfound += rpd.OriginalGoal * probability;
            probability *= (1 - rpd.OriginalGoal) * 0.85;
        }
        return pfound;
    }

    TFloatType DCG(TVector<TRankingPredictionData>& query, double TRankingPredictionData::* sortField) {
        SortQuery(query, sortField);
        TFloatType dcg = TFloatType();
        for (size_t position = 0; position < query.size(); ++position) {
            const TRankingPredictionData& rpd = query[position];
            dcg += rpd.OriginalGoal / (Log2(position + 1.) + 1);
        }
        return dcg;
    }

    TFloatType TopScore(TVector<TRankingPredictionData>& query, double TRankingPredictionData::* sortField, size_t limit) {
        SortQuery(query, sortField, limit);
        TFloatType topScore = 0;
        for (size_t position = 0; position < query.size() && position < limit; ++position) {
            topScore += query[position].OriginalGoal;
        }
        return topScore;
    }
};

template <typename TFloatType>
class TPairwiseMetricsCalculator {
private:
    TFloatType SumLogLosses = TFloatType();
    TFloatType SumCorrectPairWeights = TFloatType();
    TFloatType SumWeights = TFloatType();
public:
    void Add(const TFloatType winnerPrediction, const TFloatType looserPrediction, TFloatType weight) {
        TFloatType margin = winnerPrediction - looserPrediction;
        SumLogLosses += FastLogError(margin) * weight;
        SumCorrectPairWeights += weight * (winnerPrediction > looserPrediction);
        SumWeights += weight;
    }

    TFloatType LogLoss() const {
        return SumWeights ? SumLogLosses / SumWeights : TFloatType();
    }

    TFloatType CorrectWeightedPairs() const {
        return SumWeights ? SumCorrectPairWeights / SumWeights : TFloatType();
    }
};

template <typename TFloatType>
class TSurplusMetricsCalculator {
private:
    TFloatType SumWins = TFloatType();
    TFloatType SumLosses = TFloatType();

    TString IWChooserType;
    double SurplusThreshold;
    double StartIntentWeight;
    bool DeflateIntentWeight;
public:
    TSurplusMetricsCalculator(const TOptions& options)
        : IWChooserType(options.SurplusIWChooserType)
        , SurplusThreshold(options.SurplusThreshold)
        , StartIntentWeight(options.StartIntentWeight)
        , DeflateIntentWeight(options.DeflateIntentWeight)
    {
    }

    void Add(const TInstance<TFloatType>& instance) {
        THolder<IIntentWeightChooser> iwChooser = CreateIntentWeightChooser(IWChooserType, SurplusThreshold, StartIntentWeight);
        for (size_t i = 0; i < instance.RandomWeights.size(); ++i) {
            iwChooser->Add(instance.RandomWeights[i], instance.PredictionsForRandomWeights[i]);
        }

        TFloatType predictedIW = StartIntentWeight;

        TMaybe<double> bestIW = iwChooser->GetBestIW();
        if (bestIW) {
            predictedIW = *bestIW;
        } else if (DeflateIntentWeight) {
            predictedIW = -10;
        }

        if (fabs(predictedIW - instance.Features.front()) < 0.005) {
            if (instance.OriginalGoal > 0) {
                SumWins += instance.OriginalGoal;
            } else {
                SumLosses -= instance.OriginalGoal;
            }
        }
    }

    TFloatType Surplus() const {
        return SumWins - SumLosses;
    }

    TFloatType Wins() const {
        return SumWins;
    }

    TFloatType Losses() const {
        return SumLosses;
    }

    TFloatType WinsFraction() const {
        return (SumWins + SumLosses) ? SumWins / (SumWins + SumLosses) : 0.;
    }
};

template <typename TFloatType>
class TBoostCoefficientChooser {
protected:
    const TOptions& Options;

    const TVector<TInstance<TFloatType> >& Instances;
    const THashSet<size_t>& TestInstanceNumbers;
    const TVector<TFloatType>& NewPredictions;
public:
    TBoostCoefficientChooser(const TOptions& options,
                             const TVector<TInstance<TFloatType> >& instances,
                             const THashSet<size_t>& testInstanceNumbers,
                             const TVector<TFloatType>& newPredictions)
        : Options(options)
        , Instances(instances)
        , TestInstanceNumbers(testInstanceNumbers)
        , NewPredictions(newPredictions)
    {
    }

    virtual ~TBoostCoefficientChooser() {
    }

    virtual double CalculateLoss(TFloatType factor) const = 0;

    TFloatType GetBestFactor() const {
        if (!Options.DoShrinkageOptimization) {
            return (TFloatType) 1.;
        }

        return FindMinimum(0., 1., [this](double point){return this->CalculateLoss(point);}, Options.ThreadsCount).OptimalPoint;
    }
private:
    void CalculateLosses(const TVector<TFloatType>& factors, TVector<TFloatType>& losses, IThreadPool& queue) const {
        Y_ASSERT(factors.size() == losses.size());

        queue.Start(Options.ThreadsCount);

        const TFloatType* factor = factors.begin();
        TFloatType* loss = losses.begin();

        for (; factor != factors.end(); ++factor, ++loss) {
            queue.SafeAddFunc([=](){
                *loss = this->CalculateLoss(*factor);
            });
        }

        queue.Stop();
    }
};

}
