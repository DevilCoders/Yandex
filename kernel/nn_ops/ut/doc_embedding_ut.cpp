#include <kernel/nn_ops/doc_embedding.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>

Y_UNIT_TEST_SUITE(TEmbeddingGeneratorTestSuite) {
    Y_UNIT_TEST_DECLARE(TEmbeddingGeneratorNormalizeUrl);
    Y_UNIT_TEST_DECLARE(TEmbeddingGeneratorToChar);
}

class TEmbeddingGeneratorTest: public NNeuralNetOps::TEmbeddingGenerator {
    Y_UNIT_TEST_FRIEND(TEmbeddingGeneratorTestSuite, TEmbeddingGeneratorNormalizeUrl);
    Y_UNIT_TEST_FRIEND(TEmbeddingGeneratorTestSuite, TEmbeddingGeneratorToChar);
};

Y_UNIT_TEST_SUITE_IMPLEMENTATION(TEmbeddingGeneratorTestSuite) {
    Y_UNIT_TEST(TEmbeddingGeneratorNormalizeUrl) {
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("yandex.ru"), "yandex.ru");
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("yandex ru"), "yandex ru");
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("?"), "");
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("%"), "");
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("#"), "");
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("&"), "");
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("="), "");
        UNIT_ASSERT_STRINGS_EQUAL(TEmbeddingGeneratorTest::NormalizeUrl("https://yandex.ru/search/?text=путин&lr=213"), "https://yandex.ru/search/");
    }

    Y_UNIT_TEST(NUtilsCompress) {
        const auto& binary01 = NDssmApplier::NUtils::Compress({0.0f, 0.5f, 1.0f, 2.0f}, 0.0f, 1.0f);
        UNIT_ASSERT_VALUES_EQUAL(binary01[0], 0);
        UNIT_ASSERT_VALUES_EQUAL(binary01[1], 127);
        UNIT_ASSERT_VALUES_EQUAL(binary01[2], 255);
        UNIT_ASSERT_VALUES_EQUAL(binary01[3], 255);

        const auto& binary0255 = NDssmApplier::NUtils::Compress({-1.0f, 0.0f, 10.0f, 20.0f, 20.5f, 255.0f}, 0.0f, 255.0f);
        UNIT_ASSERT_VALUES_EQUAL(binary0255[0], 0);
        UNIT_ASSERT_VALUES_EQUAL(binary0255[1], 0);
        UNIT_ASSERT_VALUES_EQUAL(binary0255[2], 10);
        UNIT_ASSERT_VALUES_EQUAL(binary0255[3], 20);
        UNIT_ASSERT_VALUES_EQUAL(binary0255[4], 20);
        UNIT_ASSERT_VALUES_EQUAL(binary0255[5], 255);
    }

    Y_UNIT_TEST(NUtilsScale) {
        const auto& binaryX127 = NDssmApplier::NUtils::Scale({-1.0f, 0.0f, 0.5f, 1.0f, 2.0f}, 0.0f, 1.0f);
        UNIT_ASSERT_VALUES_EQUAL(binaryX127[0], 0);
        UNIT_ASSERT_VALUES_EQUAL(binaryX127[1], 0);
        UNIT_ASSERT_VALUES_EQUAL(binaryX127[2], 63);
        UNIT_ASSERT_VALUES_EQUAL(binaryX127[3], 127);
        UNIT_ASSERT_VALUES_EQUAL(binaryX127[4], 127);

        const auto& binaryX1 = NDssmApplier::NUtils::Scale({-10.0f, -1.0f, 0.0f, 10.4f, 10.5f, 10.6f, 128.0f}, -15.0f, 127.0f);
        UNIT_ASSERT_VALUES_EQUAL(binaryX1[0], -10);
        UNIT_ASSERT_VALUES_EQUAL(binaryX1[1], -1);
        UNIT_ASSERT_VALUES_EQUAL(binaryX1[2], 0);
        UNIT_ASSERT_VALUES_EQUAL(binaryX1[3], 10); // round 10.4
        UNIT_ASSERT_VALUES_EQUAL(binaryX1[4], 10); // round 10.5
        UNIT_ASSERT_VALUES_EQUAL(binaryX1[5], 11); // round 10.6
        UNIT_ASSERT_VALUES_EQUAL(binaryX1[6], 127);

        const auto& binaryX2 = NDssmApplier::NUtils::Scale({-60.0f, 0.0f, 3.1f}, -63.5f, 5.0f);
        UNIT_ASSERT_VALUES_EQUAL(binaryX2[0], -120);
        UNIT_ASSERT_VALUES_EQUAL(binaryX2[1], -0);
        UNIT_ASSERT_VALUES_EQUAL(binaryX2[2], 6);
    }
}
