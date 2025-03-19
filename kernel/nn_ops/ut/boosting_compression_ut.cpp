#include <kernel/nn_ops/boosting_compression.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>
#include <util/generic/xrange.h>

const double EPS = 1e-6;

Y_UNIT_TEST_SUITE(BoostingCompressionTestSuite) {
    Y_UNIT_TEST(Compress) {
        const auto& binary = NNeuralNetOps::Compress({-1.0f, 0.0f, 1.0f}, {0.0f, 0.5f, 1.0f}, true);
        UNIT_ASSERT_VALUES_EQUAL(binary[0], 131);
        UNIT_ASSERT_VALUES_EQUAL(binary[1], 0);
        UNIT_ASSERT_VALUES_EQUAL(binary[2], 127);
        UNIT_ASSERT_VALUES_EQUAL(binary[3], 255);
        UNIT_ASSERT_VALUES_EQUAL(binary[4], 0);
        UNIT_ASSERT_VALUES_EQUAL(binary[5], 127);
        UNIT_ASSERT_VALUES_EQUAL(binary[6], 255);
    }

    Y_UNIT_TEST(CompressDecompress) {
        const TVector<float> embedding = {0.0123f, 0.123f, -0.2121f, 0.211f};
        const TVector<float> weights = {0.111f, 0.321f};
        TVector<float> decompressedEmbedding;
        TVector<float> decompressedWeights;
        TMaybe<float> norm;
        const auto& binary = NNeuralNetOps::Compress(embedding, weights, true);
        NNeuralNetOps::Decompress(binary, decompressedEmbedding, decompressedWeights, norm, embedding.size());

        UNIT_ASSERT_VALUES_EQUAL(embedding.size(), decompressedEmbedding.size());
        UNIT_ASSERT_VALUES_EQUAL(weights.size(), decompressedWeights.size());
        for (auto i : xrange(embedding.size())) {
            UNIT_ASSERT_DOUBLES_EQUAL(embedding[i], decompressedEmbedding[i], 2.0f / 256.f / 2.f);
        }
        for (auto i : xrange(weights.size())) {
            UNIT_ASSERT_DOUBLES_EQUAL(weights[i], decompressedWeights[i], 1.0f / 256.f / 2.f);
        }
        UNIT_ASSERT(norm.Defined());
        UNIT_ASSERT_DOUBLES_EQUAL(sqrt(DotProduct(decompressedEmbedding.data(), decompressedEmbedding.data(), decompressedEmbedding.size())), norm.GetRef(), EPS);
    }

    Y_UNIT_TEST(CompressDecompressNoNorm) {
        const TVector<float> embedding = {0.0123f, 0.123f, -0.2121f, 0.211f};
        const TVector<float> weights = {0.111f, 0.321f};

        TVector<float> decompressedEmbedding;
        TVector<float> decompressedWeights;
        TMaybe<float> norm;
        const auto& binary = NNeuralNetOps::Compress(embedding, weights, false);
        NNeuralNetOps::Decompress(binary, decompressedEmbedding, decompressedWeights, norm, embedding.size());
        UNIT_ASSERT_VALUES_EQUAL(embedding.size(), decompressedEmbedding.size());
        UNIT_ASSERT_VALUES_EQUAL(weights.size(), decompressedWeights.size());
        for (auto i : xrange(embedding.size())) {
            UNIT_ASSERT_DOUBLES_EQUAL(embedding[i], decompressedEmbedding[i], 2.0f / 256.f / 2.f);
        }
        for (auto i : xrange(weights.size())) {
            UNIT_ASSERT_DOUBLES_EQUAL(weights[i], decompressedWeights[i], 1.0f / 256.f / 2.f);
        }
        UNIT_ASSERT(!norm.Defined());
    }
}
