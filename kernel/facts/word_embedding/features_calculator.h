#pragma once

#include "dictionary.h"

#include <kernel/facts/factors_info/factor_names.h>

#include <util/generic/vector.h>
#include <util/generic/hash_set.h>

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {

        class TFeaturesCalculator {
        public:
            explicit TFeaturesCalculator(TVector<EFactorId>&& factorIds, size_t embeddingSize)
                : FactorIds(std::move(factorIds))
                , EmbeddingSize(embeddingSize)
            {}
            virtual ~TFeaturesCalculator() {}
            virtual void Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const = 0;
        protected:
            TVector<EFactorId> FactorIds;
            size_t EmbeddingSize;
        };

        enum class EPairwiseStat : size_t {
            GlobalMin           /* "global_min" */,
            GlobalMax           /* "global_max" */,
            LeftMinMax          /* "left_min_max" */,
            LeftMaxMin          /* "left_max_min" */,
            RightMinMax         /* "right_min_max" */,
            RightMaxMin         /* "right_max_min" */
        };

        class TPairwiseSimStatsCalculator : public TFeaturesCalculator {
        public:
            TPairwiseSimStatsCalculator(TVector<EFactorId>&& factorIds, const TVector<EPairwiseStat>&& features,
                                        size_t leftPrefixLimit, size_t rightPrefixLimit, size_t windowSize, size_t embeddingSize)
                : TFeaturesCalculator(std::move(factorIds), embeddingSize)
                , Features(std::move(features))
                , LeftPrefixLimit(leftPrefixLimit)
                , RightPrefixLimit(rightPrefixLimit)
                , WindowSize(windowSize) {}

            void Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const override;
        private:
            TVector<EPairwiseStat> Features;
            size_t LeftPrefixLimit;
            size_t RightPrefixLimit;
            size_t WindowSize;
        };

        class TSumSimCalculator : public TFeaturesCalculator {
        public:
            TSumSimCalculator(TVector<EFactorId>&& factorIds, size_t embeddingSize) : TFeaturesCalculator(std::move(factorIds), embeddingSize) {}
            void Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const override;
        };

        class TMeanEuclideanDistCalculator : public TFeaturesCalculator {
        public:
            TMeanEuclideanDistCalculator(TVector<EFactorId>&& factorIds, size_t embeddingSize) : TFeaturesCalculator(std::move(factorIds), embeddingSize) {}
            void Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const override;
        };

        class TVectorsToSumSimCalculator : public TFeaturesCalculator {
        public:
            TVectorsToSumSimCalculator(TVector<EFactorId>&& factorIds, const TVector<TEmbedding>& vectors, size_t embeddingSize)
                : TFeaturesCalculator(std::move(factorIds), embeddingSize), Vectors(vectors) {}
            void Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const override;
        private:
            TVector<TEmbedding> Vectors;
        };
    }
}
