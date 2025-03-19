#include "features_calculator.h"

#include <util/generic/algorithm.h>

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {

        void TPairwiseSimStatsCalculator::Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const {
            size_t leftPrefixLength = Min(LeftPrefixLimit, leftInput.size());
            size_t rightPrefixLength = Min(RightPrefixLimit, rightInput.size());

            size_t leftPosCount = leftPrefixLength - Min(leftPrefixLength, WindowSize - 1);
            size_t rightPosCount = rightPrefixLength - Min(rightPrefixLength, WindowSize - 1);

            // Max(1, ...) used for code simplification
            TVector<float> leftMin(Max(static_cast<size_t>(1), leftPosCount), 1);
            TVector<float> leftMax(Max(static_cast<size_t>(1), leftPosCount), -1);
            TVector<float> rightMin(Max(static_cast<size_t>(1), rightPosCount), 1);
            TVector<float> rightMax(Max(static_cast<size_t>(1), rightPosCount), -1);

            TPartialEmbeddingSum leftPartialSum(leftInput, EmbeddingSize);
            TPartialEmbeddingSum rightPartialSum(rightInput, EmbeddingSize);

            for (size_t i = 0; i < leftPosCount; i++) {
                for(size_t j = 0; j < rightPosCount; j++) {
                    TEmbedding leftWindow, rightWindow;
                    leftPartialSum.GetSum(i, WindowSize, leftWindow);
                    rightPartialSum.GetSum(j, WindowSize, rightWindow);

                    float cosine = Cosine(leftWindow, rightWindow);
                    leftMin[i] = Min(leftMin[i], cosine);
                    leftMax[i] = Max(leftMax[i], cosine);
                    rightMin[j] = Min(rightMin[j], cosine);
                    rightMax[j] = Max(rightMax[j], cosine);
                }
            }

            Y_ENSURE(Features.size() == FactorIds.size());

            for (size_t i = 0; i < Features.size(); i++) {
                EPairwiseStat feature = Features[i];
                EFactorId factorId = FactorIds[i];
                if (feature == EPairwiseStat::GlobalMax) {
                    features[factorId] = *MaxElement(leftMax.begin(), leftMax.end());
                } else if (feature == EPairwiseStat::GlobalMin) {
                    features[factorId] = *MinElement(leftMin.begin(), leftMin.end());
                } else if (feature == EPairwiseStat::LeftMinMax) {
                    features[factorId] = *MinElement(leftMax.begin(), leftMax.end());
                } else if (feature == EPairwiseStat::RightMinMax) {
                    features[factorId] = *MinElement(rightMax.begin(), rightMax.end());
                } else if (feature == EPairwiseStat::LeftMaxMin) {
                    features[factorId] = *MaxElement(leftMin.begin(), leftMin.end());
                } else if (feature == EPairwiseStat::RightMaxMin) {
                    features[factorId] = *MaxElement(rightMin.begin(), rightMin.end());
                }
            }
        }

        void TSumSimCalculator::Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const {
            TEmbedding leftSum, rightSum;
            Sum(leftInput, leftSum, EmbeddingSize);
            Sum(rightInput, rightSum, EmbeddingSize);

            Y_ENSURE(FactorIds.size() == 1);
            features[FactorIds[0]] = Cosine(leftSum, rightSum);
        }

        void TMeanEuclideanDistCalculator::Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const {
            TEmbedding leftMean, rightMean;
            Mean(leftInput, leftMean, EmbeddingSize);
            Mean(rightInput, rightMean, EmbeddingSize);
            Subtract(leftMean, rightMean);

            Y_ENSURE(FactorIds.size() == 1);
            features[FactorIds[0]] = L2Norm(leftMean);
        }

        void TVectorsToSumSimCalculator::Calculate(TVector<TEmbedding>& leftInput, TVector<TEmbedding>& rightInput, TFactFactorStorage& features) const {
            TEmbedding leftSum, rightSum;
            Sum(leftInput, leftSum, EmbeddingSize);
            Sum(rightInput, rightSum, EmbeddingSize);

            Y_ENSURE(FactorIds.size() == 2 * Vectors.size());
            size_t factorIdIndex = 0;
            for (const TEmbedding& v : Vectors) {
                features[FactorIds[factorIdIndex++]] = Cosine(leftSum, v);
                features[FactorIds[factorIdIndex++]] = Cosine(rightSum, v);
            }
        }
    }
}
