#include "range.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TByteRangeTest)
{
    Y_UNIT_TEST(UnalignedTest)
    {
        TByteRange byteRange0(1034, 10, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(10, byteRange0.UnalignedHeadLength());
        UNIT_ASSERT_VALUES_EQUAL(0, byteRange0.UnalignedTailLength());
        UNIT_ASSERT_VALUES_EQUAL(0, byteRange0.AlignedBlockCount());
        UNIT_ASSERT_VALUES_EQUAL(1, byteRange0.BlockCount());
        UNIT_ASSERT_VALUES_EQUAL(0, byteRange0.FirstBlock());

        TByteRange byteRange1(1034, 10 + 4_KB, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(4_KB - 1034, byteRange1.UnalignedHeadLength());
        UNIT_ASSERT_VALUES_EQUAL(1044, byteRange1.UnalignedTailLength());
        UNIT_ASSERT_VALUES_EQUAL(0, byteRange1.AlignedBlockCount());
        UNIT_ASSERT_VALUES_EQUAL(2, byteRange1.BlockCount());
        UNIT_ASSERT_VALUES_EQUAL(0, byteRange1.FirstBlock());
        UNIT_ASSERT_VALUES_EQUAL(1, byteRange1.LastBlock());

        TByteRange byteRange2(1034, 10 + 8_KB, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(4_KB - 1034, byteRange2.UnalignedHeadLength());
        UNIT_ASSERT_VALUES_EQUAL(1044, byteRange2.UnalignedTailLength());
        UNIT_ASSERT_VALUES_EQUAL(1, byteRange2.AlignedBlockCount());
        UNIT_ASSERT_VALUES_EQUAL(1, byteRange2.FirstAlignedBlock());
        UNIT_ASSERT_VALUES_EQUAL(3, byteRange2.BlockCount());
        UNIT_ASSERT_VALUES_EQUAL(0, byteRange2.FirstBlock());
        UNIT_ASSERT_VALUES_EQUAL(2, byteRange2.LastBlock());
    }

    Y_UNIT_TEST(AlignedSuperRange)
    {
        auto range1 = TByteRange{1_KB, 1_KB, 4_KB}.AlignedSuperRange();
        UNIT_ASSERT_VALUES_EQUAL(range1.Offset, 0);
        UNIT_ASSERT_VALUES_EQUAL(range1.Length, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(range1.BlockSize, 4_KB);
        UNIT_ASSERT(range1.IsAligned());

        auto range2 = TByteRange{6_KB, 5_KB, 4_KB}.AlignedSuperRange();
        UNIT_ASSERT_VALUES_EQUAL(range2.Offset, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(range2.Length, 8_KB);
        UNIT_ASSERT_VALUES_EQUAL(range2.BlockSize, 4_KB);
        UNIT_ASSERT(range2.IsAligned());
    }
}

}   // namespace NCloud::NFileStore::NStorage
