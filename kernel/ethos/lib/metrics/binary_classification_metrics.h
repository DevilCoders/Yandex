#pragma once

#include <kernel/ethos/lib/data/dataset.h>

#include <library/cpp/accurate_accumulate/accurate_accumulate.h>
#include <library/cpp/scheme/scheme.h>

namespace NEthos {

struct TBcMetrics {
    double Precision = 0.;
    double Recall = 0.;
    double FalsePositiveRate = 0.;
    double F1 = 0.;
    double BestF1 = 0.;
    double Accuracy = 0.;
    double AUC = 0.;

    void DivideAndAdd(const TBcMetrics& metrics, double divisor) {
        Precision += metrics.Precision / divisor;
        Recall += metrics.Recall / divisor;
        FalsePositiveRate += metrics.FalsePositiveRate / divisor;
        F1 += metrics.F1 / divisor;
        BestF1 += metrics.BestF1 / divisor;
        Accuracy += metrics.Accuracy / divisor;
        AUC += metrics.AUC / divisor;
    }

    NSc::TValue ToTValue() const {
        NSc::TValue json;
        json["Precision"] = Precision;
        json["Recall"] = Recall;
        json["FPRate"] = FalsePositiveRate;
        json["F1"] = F1;
        json["BestF1"] = BestF1;
        json["Accuracy"] = Accuracy;
        json["AUC"] = AUC;
        return json;
    }
};

class TBcSimpleMetricsCalculator {
protected:
    TKahanAccumulator<double> TruePositivesCount;
    TKahanAccumulator<double> FalsePositivesCount;

    TKahanAccumulator<double> TrueNegativesCount;
    TKahanAccumulator<double> FalseNegativesCount;

    TKahanAccumulator<double> RightAnswersCount;

    TKahanAccumulator<double> InstancesCount;
public:
    Y_SAVELOAD_DEFINE(TruePositivesCount,
                    FalsePositivesCount,
                    TrueNegativesCount,
                    FalseNegativesCount,
                    RightAnswersCount,
                    InstancesCount);

    TBcSimpleMetricsCalculator& operator += (const TBcSimpleMetricsCalculator& other) {
        TruePositivesCount += other.TruePositivesCount;
        FalsePositivesCount += other.FalsePositivesCount;

        TrueNegativesCount += other.TrueNegativesCount;
        FalseNegativesCount += other.FalseNegativesCount;

        RightAnswersCount += other.RightAnswersCount;

        InstancesCount += other.InstancesCount;

        return *this;
    }

    void Add(EBinaryClassLabel predictedLabel, EBinaryClassLabel goalLabel, const double weight = 1.) {
        if (goalLabel == EBinaryClassLabel::BCL_UNKNOWN) {
            return;
        }

        RightAnswersCount += weight * (goalLabel == predictedLabel);
        InstancesCount += weight;

        if (predictedLabel == EBinaryClassLabel::BCL_POSITIVE) {
            TruePositivesCount  += weight * (goalLabel == EBinaryClassLabel::BCL_POSITIVE);
            FalsePositivesCount += weight * (goalLabel == EBinaryClassLabel::BCL_NEGATIVE);
        }
        if (predictedLabel == EBinaryClassLabel::BCL_NEGATIVE) {
            TrueNegativesCount  += weight * (goalLabel == EBinaryClassLabel::BCL_NEGATIVE);
            FalseNegativesCount += weight * (goalLabel == EBinaryClassLabel::BCL_POSITIVE);
        }
    }

    void Add(const TBinaryLabelWithFloatPrediction& prediction, EBinaryClassLabel goalLabel, const double weight = 1.) {
        Add(prediction.Label, goalLabel, weight);
    }

    double Precision() const {
        return Normalize(TruePositivesCount, TruePositivesCount + FalsePositivesCount);
    }

    double Recall() const {
        return Normalize(TruePositivesCount, TruePositivesCount + FalseNegativesCount);
    }

    double FalsePositiveRate() const {
        return Normalize(FalsePositivesCount, FalsePositivesCount + TrueNegativesCount);
    }

    double F1() const {
        return Normalize(2 * TruePositivesCount.Get(), (FalseNegativesCount + FalsePositivesCount + 2 * TruePositivesCount.Get()).Get());
    }

    double Accuracy() const {
        return Normalize(RightAnswersCount, InstancesCount);
    }

    double GetInstancesCount() const {
        return InstancesCount;
    }

    double GetPositivePredictionsCount() const {
        return TruePositivesCount + FalsePositivesCount;
    }
protected:
    static inline double Normalize(const double value, const double normalizer) {
        return normalizer ? (double) value / normalizer : 0.;
    }
};

class TBcMetricsCalculator : public TBcSimpleMetricsCalculator {
private:
    using TBase = TBcSimpleMetricsCalculator;

    struct TPredictionData {
        double Prediction;
        double Weight;
        EBinaryClassLabel Goal;

        bool operator < (const TPredictionData& other) const {
            if (Prediction == other.Prediction) {
                return Goal == EBinaryClassLabel::BCL_POSITIVE && other.Goal == EBinaryClassLabel::BCL_NEGATIVE;
            }
            return Prediction < other.Prediction;
        }

        bool operator > (const TPredictionData& other) const {
            return other < *this;
        }
    };

    TVector<TPredictionData> Predictions;
public:
    TBcMetricsCalculator& operator += (const TBcMetricsCalculator& other) {
        TBase::operator+=(other);
        Predictions.insert(Predictions.end(), other.Predictions.begin(), other.Predictions.end());
        return *this;
    }

    void Add(double prediction, EBinaryClassLabel predictedLabel, EBinaryClassLabel goalLabel, const double weight = 1.) {
        if (goalLabel == EBinaryClassLabel::BCL_UNKNOWN) {
            return;
        }
        TBase::Add(predictedLabel, goalLabel, weight);
        Predictions.push_back(TPredictionData{prediction, weight, goalLabel});
    }

    void Add(const TBinaryLabelWithFloatPrediction& prediction, EBinaryClassLabel goalLabel, const double weight = 1.) {
        Add(prediction.Prediction, prediction.Label, goalLabel, weight);
    }

    double BestF1(double* bestThreshold = nullptr) const {
        double actualPositivesCount = TBase::TruePositivesCount + TBase::FalseNegativesCount;
        if (!actualPositivesCount || actualPositivesCount == Predictions.size()) {
            if (bestThreshold) {
                *bestThreshold = Min<double>();
            }
            return actualPositivesCount ? 1. : 0.;
        }

        TVector<TPredictionData> sortedPredictions(Predictions);
        Sort(sortedPredictions.begin(), sortedPredictions.end(), TGreater<>());

        double bestF1 = 2. * actualPositivesCount / (TBase::InstancesCount + actualPositivesCount).Get();
        if (bestThreshold) {
            *bestThreshold = Min<double>();
        }

        TKahanAccumulator<double> truePositives;
        TKahanAccumulator<double> sumWeights;
        for (size_t divisionPosition = 1; divisionPosition < sortedPredictions.size(); ++divisionPosition) {
            const TPredictionData& currentPredictionData = sortedPredictions[divisionPosition - 1];
            const TPredictionData& nextPredictionData = sortedPredictions[divisionPosition - 0];

            sumWeights += currentPredictionData.Weight;
            truePositives += currentPredictionData.Weight * (currentPredictionData.Goal == EBinaryClassLabel::BCL_POSITIVE);
            if (!truePositives.Get() || nextPredictionData.Prediction + 1e-6 > currentPredictionData.Prediction) {
                continue;
            }

            double f1 = 2. * truePositives.Get() / (sumWeights + actualPositivesCount).Get();
            if (bestThreshold && f1 > bestF1) {
                *bestThreshold = (currentPredictionData.Prediction + nextPredictionData.Prediction) / 2;
            }
            bestF1 = Max(f1, bestF1);
        }

        return bestF1;
    }

    double AUC() const {
        TVector<TPredictionData> sortedPredictions(Predictions);
        Sort(sortedPredictions.begin(), sortedPredictions.end());

        TKahanAccumulator<double> sumWeights;
        TKahanAccumulator<double> sumNegativeWeights;
        for (size_t position = 0; position < sortedPredictions.size(); ++position) {
            const TPredictionData& prediction = sortedPredictions[position];
            Y_ASSERT(prediction.Goal != EBinaryClassLabel::BCL_UNKNOWN);

            if (prediction.Goal == EBinaryClassLabel::BCL_POSITIVE) {
                sumWeights += sumNegativeWeights * prediction.Weight;
            } else {
                sumNegativeWeights += prediction.Weight;
            }
        }

        double actualPositivesCount = TBase::TruePositivesCount + TBase::FalseNegativesCount;
        double actualNegativesCount = TBase::TrueNegativesCount + TBase::FalsePositivesCount;

        if (!actualNegativesCount || !actualPositivesCount) {
            return 0.;
        }

        return sumWeights / actualPositivesCount / actualNegativesCount;
    }

    double BestThreshold() const {
        double bestThreshold;
        BestF1(&bestThreshold);
        return bestThreshold;
    }

    TBcMetrics AllMetrics() {
        TBcMetrics metrics;
        metrics.Precision = TBase::Precision();
        metrics.Recall = TBase::Recall();
        metrics.FalsePositiveRate = TBase::FalsePositiveRate();
        metrics.F1 = TBase::F1();
        metrics.BestF1 = BestF1();
        metrics.Accuracy = TBase::Accuracy();
        metrics.AUC = AUC();
        return metrics;
    }
};

}
