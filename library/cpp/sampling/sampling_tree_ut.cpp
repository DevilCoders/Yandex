#include "sampling_tree.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ymath.h>
#include <util/random/mersenne.h>

template <typename TFreq>
static void DoSamplingTreeTest() {
    TMersenne<ui64> generator;

    // Setup a simple sampling tree
    NSampling::TSamplingTree<TFreq> tree(3, 1);
    tree.SetFreq(1, 0);
    tree.SetFreq(2, 2);

    // Make sure the retrieved frequencies are correct
    UNIT_ASSERT_EQUAL(tree.GetFreq(0), 1);
    UNIT_ASSERT_EQUAL(tree.GetFreq(1), 0);
    UNIT_ASSERT_EQUAL(tree.GetFreq(2), 2);

    // Compute sample frequencies
    TFreq countOutcomes[3] = {0};
    const size_t sampleCount = 10000;
    for (size_t i = 0; i < sampleCount; ++i) {
        size_t sample = tree(generator);
        UNIT_ASSERT(sample < 3);
        ++countOutcomes[sample];
    }

    // Make sure we've sampled from the right distribution
    UNIT_ASSERT(Abs(static_cast<double>(countOutcomes[0]) / sampleCount - 1.0 / 3.0) < 1e-2);
    UNIT_ASSERT_EQUAL(countOutcomes[1], 0);
    UNIT_ASSERT(Abs(static_cast<double>(countOutcomes[2]) / sampleCount - 2.0 / 3.0) < 1e-2);

    // Make sure we can change frequencies on the fly
    tree.SetFreq(0, 0);
    for (size_t i = 0; i < 10; ++i) {
        UNIT_ASSERT_EQUAL(tree(generator), 2);
    }

    // Make sure we can sample from a tree with just one element
    NSampling::TSamplingTree<TFreq> treeOneElem(1, 10);
    for (size_t i = 0; i < 10; ++i) {
        UNIT_ASSERT_EQUAL(treeOneElem(generator), 0);
    }
}

Y_UNIT_TEST_SUITE(SamplingTreeTest) {
    Y_UNIT_TEST(TestUI32SamplingTree) {
        DoSamplingTreeTest<ui32>();
    }

    Y_UNIT_TEST(TestUI64SamplingTree) {
        DoSamplingTreeTest<ui64>();
    }

    Y_UNIT_TEST(TestFloatSamplingTree) {
        DoSamplingTreeTest<float>();
    }

    Y_UNIT_TEST(TestDoubleSamplingTree) {
        DoSamplingTreeTest<double>();
    }
}
