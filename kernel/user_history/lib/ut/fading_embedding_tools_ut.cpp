#include "fading_embedding_tools.h"

#include <kernel/embeddings_info/lib/tools.h>
#include <kernel/user_history/proto/fading_embedding.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/dot_product/dot_product.h>

#include <util/generic/xrange.h>
#include <util/generic/ymath.h>
#include <util/random/random.h>
#include <util/string/builder.h>

using namespace NPersonalization;

namespace {
    TVector<float> GenerateRandomEmbedding(size_t len) {
        float l2Norm = 0.0;
        TVector<float> result(Reserve(len));
        do {
            result.clear();
            for (auto i: xrange(len)) {
                Y_UNUSED(i);
                result.push_back(RandomNumber<float>()*2 - 1.0);
            }
            l2Norm = L2NormSquared(&result[0], result.size());
        } while (l2Norm < 0.01);

        // Forcing l2norm == 1
        for (auto& v: result) {
            v /= std::sqrt(l2Norm);
        }
        return result;
    }
}

Y_UNIT_TEST_SUITE(FadingEmbedding) {
    Y_UNIT_TEST(UpdateFadingEmbedding) {
        NProto::TFadingEmbedding fadingEmbedding;
        fadingEmbedding.MutableEmbedding()->SetFormat(NEmbedding::TEmbedding::Decompressed);
        fadingEmbedding.SetFadingCoefDays(0.5);

        const size_t len = 50;
        const size_t tries = 256;

        for (auto i: xrange(tries)) {
            const auto ts = i * 3600;

            auto randomEmbedding = GenerateRandomEmbedding(len);
            {
                UNIT_ASSERT(
                    AllOf(randomEmbedding.begin(), randomEmbedding.end(), IsValidFloat)
                );

                UNIT_ASSERT_DOUBLES_EQUAL(
                    L2NormSquared(&randomEmbedding[0], randomEmbedding.size()),
                    1.0,
                    0.00001
                );
            }
            const auto forcedCopy = randomEmbedding;

            {
                TVector<TArrayRef<float>> embeddings(Reserve(1));
                TVector<ui64> timestamps(1, ts);
                embeddings.push_back(randomEmbedding);
                UpdateFadingEmbedding(fadingEmbedding, embeddings, timestamps);
            }

            // UpdateFadingEmbedding does not modify original vector
            UNIT_ASSERT(forcedCopy.size() == randomEmbedding.size());
            UNIT_ASSERT(Equal(forcedCopy.begin(), forcedCopy.end(), randomEmbedding.begin()));

            for (auto& v: randomEmbedding) {
                v *= -1.0;
            }

            // reverse update
            {
                TVector<TArrayRef<float>> embeddings(Reserve(1));
                TVector<ui64> timestamps(1, ts);
                embeddings.push_back(randomEmbedding);
                UpdateFadingEmbedding(fadingEmbedding, embeddings, timestamps);
            }

            // Ideally we would have zero vector at this point
            {
                TVector<float> zeroVec = NEmbeddingTools::GetVector(fadingEmbedding.GetEmbedding());
                UNIT_ASSERT_EQUAL(zeroVec.size(), len);
                UNIT_ASSERT_DOUBLES_EQUAL_C(
                    L2NormSquared(&zeroVec[0], zeroVec.size()),
                    0.0,
                    0.001,
                    TStringBuilder() << "Lost enough precision on iter=" << ToString(i)
                );
            }
        }
    }
}
