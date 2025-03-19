#include "pca.h"

#include <library/cpp/linear_regression/linear_regression.h>

#include <util/random/mersenne.h>
#include <util/random/shuffle.h>

#include <util/system/mutex.h>
#include <util/thread/pool.h>

namespace NEthosAutoEncoder {

double IterateWeights(const TVector<TVector<double> >& dataMatrix,
                      const TVector<double>& columnWeights,
                      TVector<double>& rowWeights,
                      const TPCALearningOptions& learningOptions)
{
    Y_ASSERT(dataMatrix.size() == rowWeights.size() && dataMatrix.front().size() == columnWeights.size());

    TVector<double> sse(rowWeights.size());

    THolder<IThreadPool> queue = CreateThreadPool(learningOptions.ThreadsCount);
    for (size_t threadNumber = 0; threadNumber < learningOptions.ThreadsCount; ++threadNumber) {
        queue->SafeAddFunc([&, threadNumber](){
            for (size_t rowNumber = threadNumber; rowNumber < rowWeights.size(); rowNumber += learningOptions.ThreadsCount) {
                TSLRSolver slrSolver;
                slrSolver.Add(columnWeights.begin(), columnWeights.end(), dataMatrix[rowNumber].begin());

                double intercept;
                slrSolver.Solve(rowWeights[rowNumber], intercept, learningOptions.RegularizationParameter);

                sse[rowNumber] += slrSolver.SumSquaredErrors();
            }
        });
    }
    queue->Stop();

    return FastKahanAccumulate(sse);
}

void InitRandomVector(TVector<double>& sequence) {
    for (size_t i = 0; i < sequence.size(); ++i) {
        sequence[i] = (double) i / sequence.size();
    }
    static TMersenne<ui64> mersenne;
    Shuffle(sequence.begin(), sequence.end(), mersenne);
}

TLinearFeaturesEncoder TLinearFeaturesEncoderLearner::Learn() {
    const size_t instancesCount = FeaturesMatrix.size();
    const size_t featuresCount = TransposedMatrix.size();

    TVector<TVector<double> > quantiles;

    switch (LearningOptions.PoolNormalizationType) {
    case EPoolNormalizationType::PN_NONE:
        break;
    case EPoolNormalizationType::PN_QUANTILE:
        quantiles.resize(featuresCount);

        for (size_t featureNumber = 0; featureNumber < featuresCount; ++featureNumber) {
            TVector<double>& featureQuantiles = quantiles[featureNumber];

            TVector<double> featuresVector = TransposedMatrix[featureNumber];
            Sort(featuresVector.begin(), featuresVector.end());

            for (size_t level = 0; level < LearningOptions.QuantilesCount; ++level) {
                featureQuantiles.push_back(featuresVector[level * featuresVector.size() / LearningOptions.QuantilesCount]);
            }
        }

        for (size_t i = 0; i < instancesCount; ++i) {
            for (size_t j = 0; j < featuresCount; ++j) {
                FeaturesMatrix[i][j] = Quantile(FeaturesMatrix[i][j], quantiles[j]);
                TransposedMatrix[j][i] = FeaturesMatrix[i][j];
            }
        }

        break;
    case EPoolNormalizationType::PN_NORMALIZE_LOGIFY:
        for (size_t i = 0; i < instancesCount; ++i) {
            for (size_t j = 0; j < featuresCount; ++j) {
                FeaturesMatrix[i][j] = Logify(Normalize(FeaturesMatrix[i][j], FeatureStatisticCalculators[j]));
                TransposedMatrix[j][i] = FeaturesMatrix[i][j];
            }
        }
        break;
    case EPoolNormalizationType::PN_NORMALIZE:
        for (size_t i = 0; i < instancesCount; ++i) {
            for (size_t j = 0; j < featuresCount; ++j) {
                FeaturesMatrix[i][j] = Normalize(FeaturesMatrix[i][j], FeatureStatisticCalculators[j]);
                TransposedMatrix[j][i] = FeaturesMatrix[i][j];
            }
        }
        break;
    case EPoolNormalizationType::PN_LOGIFY:
        for (size_t i = 0; i < instancesCount; ++i) {
            for (size_t j = 0; j < featuresCount; ++j) {
                FeaturesMatrix[i][j] = Logify(FeaturesMatrix[i][j]);
                TransposedMatrix[j][i] = FeaturesMatrix[i][j];
            }
        }
        break;
    }

    const size_t actualTransformationsCount = LearningOptions.TransformationsCount ? Min(LearningOptions.TransformationsCount, featuresCount)
                                                                                   : featuresCount / 2;

    TVector<TVector<double> > transformations(actualTransformationsCount, TVector<double>(featuresCount));

    TMersenne<ui64> mersenne;
    for (size_t iterationNumber = 0; iterationNumber < LearningOptions.IterationsCount; ++iterationNumber) {
        Cout << "iteration #" << (iterationNumber + 1) << ":" << Endl;

        for (size_t transformationNumber = 0; transformationNumber < actualTransformationsCount; ++transformationNumber) {
            TVector<double>& transformation = transformations[transformationNumber];
            InitRandomVector(transformation);

            TVector<double> instanceFactors(instancesCount);
            InitRandomVector(instanceFactors);

            IterateWeights(FeaturesMatrix, transformation, instanceFactors, LearningOptions);
            double sse = IterateWeights(TransposedMatrix, instanceFactors, transformation, LearningOptions);

            Cout << "transformation #" << (transformationNumber + 1) << ": " << sqrt(sse / featuresCount / featuresCount) << Endl;

            THolder<IThreadPool> queue = CreateThreadPool(LearningOptions.ThreadsCount);
            for (size_t threadNumber = 0; threadNumber < LearningOptions.ThreadsCount; ++threadNumber) {
                queue->SafeAddFunc([&, threadNumber](){
                    for (size_t i = threadNumber; i < instancesCount; i += LearningOptions.ThreadsCount) {
                        for (size_t j = 0; j < featuresCount; ++j) {
                            FeaturesMatrix[i][j] -= instanceFactors[i] * transformation[j];
                            TransposedMatrix[j][i] -= instanceFactors[i] * transformation[j];
                        }
                    }
                });
            }
            queue->Stop();
        }
    }

    if (LearningOptions.PoolNormalizationType == EPoolNormalizationType::PN_NORMALIZE) {
        for (TVector<double>& transformation : transformations) {
            for (size_t featureNumber = 0; featureNumber < featuresCount; ++featureNumber) {
                if (FeatureStatisticCalculators[featureNumber].GetStdDev() > 1e-5) {
                    transformation[featureNumber] /= FeatureStatisticCalculators[featureNumber].GetStdDev();
                }
            }
        }
    }

    return TLinearFeaturesEncoder(LearningOptions, FeatureStatisticCalculators, std::move(quantiles), std::move(transformations));
}

}
