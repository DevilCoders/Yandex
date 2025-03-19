#pragma once
#include <util/generic/vector.h>

namespace NUnstructuredFeatures
{
    struct TEmbedding {
        TVector<float> Vec;
        TVector<float> NormVec;

        TEmbedding() {}
        TEmbedding(const TVector<float> &vec) {
            Assign(TVector<float>(vec));
        }
        TEmbedding(TVector<float>&& vec) {
            Assign(std::move(vec));
        }
        bool Empty() const {
            return Vec.empty();
        }
        void Assign(TVector<float> &&vec);
    };

    float Dot(const TEmbedding &lhs, const TEmbedding &rhs);
    float Cosine(const TEmbedding &lhs, const TEmbedding &rhs);
    float L2Norm(const TEmbedding &embedding);

    void Sum(const TVector<TEmbedding>& embeddings, TEmbedding& sum, size_t embeddingSize);
    void Multiply(TEmbedding& embedding, float multiplier);
    void Subtract(TEmbedding& lhs, const TEmbedding& rhs);

    void Mean(const TVector<TEmbedding>& embeddings, TEmbedding& mean, size_t embeddingSize);

    class TPartialEmbeddingSum {
    public:
        TPartialEmbeddingSum(const TVector<TEmbedding>& embeddings, size_t embeddingSize);
        void GetSum(size_t from, size_t length, TEmbedding& sum) const;
    private:
        TVector<TVector<float>> PartialVecSum;
    };
}
