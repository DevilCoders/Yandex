#include "utils.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

Y_UNIT_TEST_SUITE(TFloat2UI8CompressorTests) {
    using namespace NDssmApplier::NUtils;
    Y_UNIT_TEST(CompressDecompressTest) {
        float multipler = 0.f;
        const TVector<float> vec = {
            -2.f, -2.01f, -2.02f, -2.03f, -1.75f, -1.73f, -1.71f, -1.69f, -1.67f, -1.65f, -1.f, -0.75f, -0.5f, -0.25f, 0.f, 0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 1.75f, 2.f};
        const TVector<ui8> expectedCompressedVec = {1, 1, 0, 0, 17, 18, 20, 21, 22, 23, 64, 80, 96, 112, 127, 143, 159, 175, 191, 206, 222, 238, 254};

        const TVector<ui8> compressedVec = TFloat2UI8Compressor::Compress(vec, &multipler);
        UNIT_ASSERT_VALUES_EQUAL(expectedCompressedVec, compressedVec);

        TVector<float> decompressedVec = TFloat2UI8Compressor::Decompress(compressedVec);
        Transform(decompressedVec.begin(), decompressedVec.end(), decompressedVec.begin(), [multipler](float f) {
            return f * multipler;
        });
        bool vectorsEqual = true;
        const float maxError = 2.f * multipler / 256.f;
        UNIT_ASSERT_VALUES_EQUAL(vec.size(), decompressedVec.size());
        for (ui64 i = 0; i < vec.size(); ++i) {
            vectorsEqual = vectorsEqual && fabs(vec[i] - decompressedVec[i]) < maxError;
        }
        UNIT_ASSERT_VALUES_EQUAL(vectorsEqual, true);
    }
}
