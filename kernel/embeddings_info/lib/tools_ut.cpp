#include "tools.h"

#include <kernel/embeddings_info/proto/embedding.pb.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/random/random.h>

namespace {
    using namespace NEmbedding;

    const float NO_COMPRESSION_ERROR = 1.0e-6;
    const float AFTER_COMPRESSION_ERROR = 5.0e-3;

    TVector<float> RandomNormalizedVector(size_t size = 100) {
        TVector<float> res(size);
        for (auto& el : res) {
            el = RandomNumber<float>();
        }
        const float norm = std::sqrtf(L2NormSquared(res.data(), res.size()));
        Transform(res.begin(), res.end(), res.begin(), [norm](float f) {
            return f / norm;
        });
        return res;
    }

    bool CompareVectors(const TVector<float>& v1, const TVector<float>& v2, const float eps) {
        if (v1.size() != v2.size()) {
            return false;
        }

        bool res = true;
        for (size_t i = 0; i < v1.size(); ++i) {
            res &= Abs(v1[i] - v2[i]) <= eps;
        }
        return res;
    }
}

Y_UNIT_TEST_SUITE(TEmbeddingTools) {
    using namespace NEmbedding;

    Y_UNIT_TEST(TestSetGet) {
        TVector<float> input = RandomNormalizedVector();
        TEmbedding emb = NEmbeddingTools::CreateEmptyEmbedding(TEmbedding_EFormat::TEmbedding_EFormat_Decompressed);
        NEmbeddingTools::SetVector(input, emb);
        TVector<float> output = NEmbeddingTools::GetVector(emb);
        UNIT_ASSERT(CompareVectors(input, output, NO_COMPRESSION_ERROR));
    }

    Y_UNIT_TEST(TestSetCompressGet) {
        TVector<float> input = RandomNormalizedVector();
        TEmbedding emb = NEmbeddingTools::CreateEmptyEmbedding(TEmbedding_EFormat::TEmbedding_EFormat_Decompressed);
        NEmbeddingTools::SetVector(input, emb);
        NEmbeddingTools::Compress(emb);
        TVector<float> output = NEmbeddingTools::GetVector(emb);
        UNIT_ASSERT(CompareVectors(input, output, AFTER_COMPRESSION_ERROR));
    }

    Y_UNIT_TEST(TestCompressSetGet) {
        TVector<float> input = RandomNormalizedVector();
        TEmbedding emb = NEmbeddingTools::CreateEmptyEmbedding(TEmbedding_EFormat::TEmbedding_EFormat_Compressed);
        NEmbeddingTools::SetVector(input, emb);
        TVector<float> output = NEmbeddingTools::GetVector(emb);
        UNIT_ASSERT(CompareVectors(input, output, AFTER_COMPRESSION_ERROR));
    }

    Y_UNIT_TEST(TestSetCompressDecompressGet) {
        TVector<float> input = RandomNormalizedVector();
        TEmbedding emb = NEmbeddingTools::CreateEmptyEmbedding(TEmbedding_EFormat::TEmbedding_EFormat_Decompressed);
        NEmbeddingTools::SetVector(input, emb);
        NEmbeddingTools::Compress(emb);
        NEmbeddingTools::Decompress(emb);
        TVector<float> output = NEmbeddingTools::GetVector(emb);
        UNIT_ASSERT(CompareVectors(input, output, AFTER_COMPRESSION_ERROR));
    }

    Y_UNIT_TEST(TestZeroL2NormCompressDecompressGet) {
        const TVector<float> input(100, 0.0);
        TEmbedding emb = NEmbeddingTools::CreateEmptyEmbedding(TEmbedding_EFormat::TEmbedding_EFormat_Decompressed);
        NEmbeddingTools::SetVector(input, emb);
        NEmbeddingTools::Compress(emb);
        NEmbeddingTools::Decompress(emb);
        const auto output = NEmbeddingTools::GetVector(emb);
        UNIT_ASSERT(CompareVectors(input, output, AFTER_COMPRESSION_ERROR));
    }
}
