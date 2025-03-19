#pragma once

#include "pool.h"

#include <kernel/ethos/lib/data/dataset.h>
#include <kernel/ethos/lib/util/any_iterator.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/linear_regression/linear_regression.h>

#include <utility>

#include <util/stream/file.h>

#include <util/ysaveload.h>

namespace NEthosAutoEncoder {

enum EPoolNormalizationType {
    PN_NONE,
    PN_NORMALIZE,
    PN_LOGIFY,
    PN_NORMALIZE_LOGIFY,
    PN_QUANTILE,
};

struct TPCALearningOptions {
    size_t IterationsCount = 100;
    size_t TransformationsCount = 0;
    size_t ThreadsCount = 10;

    size_t QuantilesCount = 16;

    EPoolNormalizationType PoolNormalizationType = EPoolNormalizationType::PN_NONE;

    double RegularizationParameter = 1e-3;

    void AddOpts(NLastGetopt::TOpts& opts) {
        opts.AddLongOption('i', "iterations", "iterations per component count")
            .Optional()
            .StoreResult(&IterationsCount);
        opts.AddLongOption('t', "threads", "learning threads count")
            .Optional()
            .StoreResult(&ThreadsCount);
        opts.AddLongOption('c', "components", "decomposition components count")
            .Optional()
            .StoreResult(&TransformationsCount);
        opts.AddLongOption('r', "regularization", "regularization parameter for simple linear regression")
            .Optional()
            .StoreResult(&RegularizationParameter);

        opts.AddLongOption("normalize", "do normalize features")
            .Optional()
            .NoArgument()
            .StoreValue(&PoolNormalizationType, EPoolNormalizationType::PN_NORMALIZE);
        opts.AddLongOption("logify", "do logify features")
            .Optional()
            .NoArgument()
            .StoreValue(&PoolNormalizationType, EPoolNormalizationType::PN_LOGIFY);
        opts.AddLongOption("normalize-logify", "do normalize, and then logify features")
            .Optional()
            .NoArgument()
            .StoreValue(&PoolNormalizationType, EPoolNormalizationType::PN_NORMALIZE_LOGIFY);
        opts.AddLongOption("quantile", "quantile normalization")
            .Optional()
            .NoArgument()
            .StoreValue(&PoolNormalizationType, EPoolNormalizationType::PN_QUANTILE);
        opts.AddLongOption("quantiles", "quantile levels count")
            .Optional()
            .StoreResult(&QuantilesCount);
    }
};

static inline double Logify(const double value) {
    return value > 0 ? log(1. + value) : -log(1. - value);
}

static inline double Normalize(const double value, const TDeviationCalculator& statistics) {
    double normalizedValue = value - statistics.GetMean();
    if (statistics.GetStdDev() > 1e-5) {
        normalizedValue /= statistics.GetStdDev();
    }
    return normalizedValue;
}

static inline double Quantile(const double value, const TVector<double>& quantiles) {
    const double offset = LowerBound(quantiles.begin(), quantiles.end(), value) - quantiles.begin();
    return offset / quantiles.size();
}

class TLinearFeaturesEncoder {
private:
    EPoolNormalizationType PoolNormalizationType;

    TVector<TDeviationCalculator> FeatureStatistics;
    TVector<TVector<double> > Quantiles;

    TVector<TVector<double> > Transformations;
public:
    Y_SAVELOAD_DEFINE(PoolNormalizationType, FeatureStatistics, Quantiles, Transformations);

    TLinearFeaturesEncoder() {
    }

    TLinearFeaturesEncoder(const TPCALearningOptions& learningOptions,
                           const TVector<TDeviationCalculator>& featureStatistics,
                           TVector<TVector<double> >&& quantiles,
                           TVector<TVector<double> >&& transformations)
        : PoolNormalizationType(learningOptions.PoolNormalizationType)
        , FeatureStatistics(featureStatistics)
        , Quantiles(std::move(quantiles))
        , Transformations(std::move(transformations))
    {
    }

    TVector<double> Encode(const TVector<double>& features) const {
        TVector<double> encodedFeatures(Transformations.size());

        for (size_t transformationNumber = 0; transformationNumber < encodedFeatures.size(); ++transformationNumber) {
            double& encodedFeature = encodedFeatures[transformationNumber];

            const TVector<double>& coefficients = Transformations[transformationNumber];

            switch (PoolNormalizationType) {
            case EPoolNormalizationType::PN_LOGIFY:
                for (size_t featureNumber = 0; featureNumber < coefficients.size(); ++featureNumber) {
                    encodedFeature += Logify(features[featureNumber]) * coefficients[featureNumber];
                }
                break;
            case EPoolNormalizationType::PN_NORMALIZE_LOGIFY:
                for (size_t featureNumber = 0; featureNumber < coefficients.size(); ++featureNumber) {
                    encodedFeature += Logify(Normalize(features[featureNumber], FeatureStatistics[featureNumber])) * coefficients[featureNumber];
                }
                break;
            case EPoolNormalizationType::PN_QUANTILE:
                for (size_t featureNumber = 0; featureNumber < coefficients.size(); ++featureNumber) {
                    encodedFeature += Quantile(features[featureNumber], Quantiles[featureNumber]) * coefficients[featureNumber];
                }
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            default:
                encodedFeature = InnerProduct(coefficients, features);
            }
        }

        return encodedFeatures;
    }
};

class TLinearFeaturesEncoderLearner {
private:
    TPCALearningOptions LearningOptions;

    TVector<TDeviationCalculator> FeatureStatisticCalculators;

    TVector<TVector<double> > FeaturesMatrix;
    TVector<TVector<double> > TransposedMatrix;
public:
    TLinearFeaturesEncoderLearner(const TPCALearningOptions& learningOptions)
        : LearningOptions(learningOptions)
    {
    }

    void Add(const TVector<double>& features) {
        FeaturesMatrix.push_back(features);
        if (TransposedMatrix.empty()) {
            TransposedMatrix.resize(features.size());
            FeatureStatisticCalculators.resize(features.size());
        }
        for (size_t i = 0; i < features.size(); ++i) {
            TransposedMatrix[i].push_back(features[i]);
            FeatureStatisticCalculators[i].Add(features[i]);
        }
    }

    TLinearFeaturesEncoder Learn();
};

static inline int LearnPCA(int argc, const char** argv) {
    NEthosAutoEncoder::TPCALearningOptions learningOptions;

    TString featuresPath;
    TString encoderModelPath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption('f', "features", "features path")
            .Required()
            .StoreResult(&featuresPath);
        opts.AddLongOption('m', "model", "resulting encoder model path")
            .Optional()
            .StoreResult(&encoderModelPath);

        learningOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TVector<TVector<double> > features = ReadFeatures(featuresPath);
    if (features.empty()) {
        Cerr << "no features found!" << Endl;
        return 1;
    }

    NEthosAutoEncoder::TLinearFeaturesEncoderLearner encoderLearner(learningOptions);
    for (const TVector<double>& instance : features) {
        encoderLearner.Add(instance);
    }

    NEthosAutoEncoder::TLinearFeaturesEncoder encoder = encoderLearner.Learn();
    if (encoderModelPath) {
        TFixedBufferFileOutput modelOut(encoderModelPath);
        encoder.Save(&modelOut);
    }

    return 0;
}

}
