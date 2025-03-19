#include <kernel/vector_machine/slicers.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NVectorMachine;

Y_UNIT_TEST_SUITE(TSlicerTest) {
    Y_UNIT_TEST(TestAllSlicer) {
        TVector<float> thresholdValues = {1.f, 1.f, 2.f, 2.f, 3.f, 3.f, 5.f};
        const TAllSlicer slicer(2.5f);
        const auto [sliceBegin, sliceSize] = slicer.GetBeginAndSize(thresholdValues, thresholdValues.size());
        UNIT_ASSERT_VALUES_EQUAL(sliceBegin, 0);
        UNIT_ASSERT_VALUES_EQUAL(sliceSize, thresholdValues.size());
    }

    Y_UNIT_TEST(TestLessSlicer) {
        TVector<float> thresholdValues = {1.f, 1.f, 2.f, 2.f, 3.f, 3.f, 5.f};
        const TLessSlicer slicer(2.f);
        const auto [sliceBegin, sliceSize] = slicer.GetBeginAndSize(thresholdValues, thresholdValues.size());
        UNIT_ASSERT_VALUES_EQUAL(sliceBegin, 0);
        UNIT_ASSERT_VALUES_EQUAL(sliceSize, 2);
    }

    Y_UNIT_TEST(TestGreaterEqualSlicer) {
        TVector<float> thresholdValues = {1.f, 1.f, 2.f, 2.f, 3.f, 3.f, 5.f};
        const TGreaterEqualSlicer slicer(2.f);
        const auto [sliceBegin, sliceSize] = slicer.GetBeginAndSize(thresholdValues, thresholdValues.size());
        UNIT_ASSERT_VALUES_EQUAL(sliceBegin, 2);
        UNIT_ASSERT_VALUES_EQUAL(sliceSize, 5);
    }
}
