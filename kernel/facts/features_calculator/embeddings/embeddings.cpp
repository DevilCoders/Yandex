#include "embeddings.h"

#include <library/cpp/dot_product/dot_product.h>

#include <util/system/yassert.h>
#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>

namespace NUnstructuredFeatures
{
    static bool TryNormalize(TEmbedding& embedding) {
        float sum = 0;
        for (float item : embedding.Vec) {
            sum += item * item;
        }
        if (sum >= std::numeric_limits<float>::epsilon()) {
            embedding.NormVec = embedding.Vec;
            float k = 1.0 / sqrt(sum);
            for (float& item : embedding.NormVec) {
                item *= k;
            }
            return true;
        }
        embedding.NormVec = TVector<float>(embedding.Vec.size(), 0.0f);
        return false;
    }

    void TEmbedding::Assign(TVector<float> &&vec) {
        Vec.swap(vec);
        TryNormalize(*this);
    }

    float Dot(const TEmbedding &lhs, const TEmbedding &rhs) {
        Y_ASSERT(!lhs.Empty());
        Y_ASSERT(lhs.Vec.size() == rhs.Vec.size());
        return ::DotProduct(lhs.Vec.begin(), rhs.Vec.begin(), lhs.Vec.size());
    }

    float Cosine(const TEmbedding &lhs, const TEmbedding &rhs) {
        Y_ASSERT(!lhs.Empty());
        Y_ASSERT(lhs.NormVec.size() == rhs.NormVec.size());
        return ::DotProduct(lhs.NormVec.begin(), rhs.NormVec.begin(), lhs.NormVec.size());
    }

    float L2Norm(const TEmbedding& embedding) {
        Y_ASSERT(!embedding.Empty());
        return ::DotProduct(embedding.Vec.begin(), embedding.Vec.begin(), embedding.Vec.size());
    }

    void Sum(const TVector<TEmbedding>& embeddings, TEmbedding& sum, size_t embeddingSize) {
        if (embeddings.empty()) {
            sum.Assign(TVector<float>(embeddingSize, 0.0f));
            return;
        }
        sum.Vec = TVector<float>(embeddingSize, 0.0f);
        for (const TEmbedding& embedding : embeddings) {
            for (size_t i = 0; i < embedding.Vec.size(); i++) {
                sum.Vec[i] += embedding.Vec[i];
            }
        }
        TryNormalize(sum);
    }

    void Mean(const TVector<TEmbedding>& embeddings, TEmbedding& mean, size_t embeddingSize) {
        Sum(embeddings, mean, embeddingSize);
        float multiplier = 1.0f / Max(static_cast<size_t>(1), embeddings.size());
        Multiply(mean, multiplier);
    }

    void Multiply(TEmbedding& embedding, float multiplier) {
        for (float& item : embedding.Vec) {
            item *= multiplier;
        }
        if (multiplier < std::numeric_limits<float>::epsilon()) {
            TryNormalize(embedding);
        }
    }

    void Subtract(TEmbedding& lhs, const TEmbedding& rhs) {
        Y_ASSERT(lhs.Vec.size() == rhs.Vec.size());
        for (size_t i = 0; i < lhs.Vec.size(); i++ ) {
            lhs.Vec[i] -= rhs.Vec[i];
        }
        TryNormalize(lhs);
    }

    TPartialEmbeddingSum::TPartialEmbeddingSum(const TVector<TEmbedding>& embeddings, size_t embeddingSize) {
        PartialVecSum.resize(embeddings.size() + 1, TVector<float>(embeddingSize, 0.0f));
        for (size_t i = 0; i < embeddings.size(); i++) {
            for (size_t j = 0; j < embeddingSize; j++) {
                PartialVecSum[i + 1][j] = PartialVecSum[i][j] + embeddings[i].Vec[j];
            }
        }
    }

    void TPartialEmbeddingSum::GetSum(size_t from, size_t length, TEmbedding& sum) const {
        Y_ASSERT(from + length < PartialVecSum.size());
        sum.Vec = PartialVecSum[from + length];
        Subtract(sum, PartialVecSum[from]);
    }
}
