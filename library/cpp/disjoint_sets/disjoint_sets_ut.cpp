#include "disjoint_sets.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TDisjointSetsTest) {
    Y_UNIT_TEST(SimpleTest) {
        TDisjointSets sets(4);

        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(0), 0);
        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(1), 1);
        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(2), 2);
        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(3), 3);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(0), 1);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(1), 1);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(2), 1);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(3), 1);
        UNIT_ASSERT_EQUAL(sets.SetCount(), 4);

        sets.UnionSets(0, 1);

        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(0), sets.CanonicSetElement(1));
        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(2), 2);
        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(3), 3);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(0), 2);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(1), 2);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(2), 1);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(3), 1);
        UNIT_ASSERT_EQUAL(sets.SetCount(), 3);

        sets.UnionSets(1, 3);

        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(0), sets.CanonicSetElement(1));
        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(0), sets.CanonicSetElement(3));
        UNIT_ASSERT_EQUAL(sets.CanonicSetElement(2), 2);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(0), 3);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(1), 3);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(2), 1);
        UNIT_ASSERT_EQUAL(sets.SizeOfSet(3), 3);
        UNIT_ASSERT_EQUAL(sets.SetCount(), 2);
    }

    Y_UNIT_TEST(UnionAllTest) {
        TDisjointSets sets(1000);
        for (size_t i = 0; i < sets.InitialSetCount(); ++i) {
            sets.UnionSets(0, i);
            UNIT_ASSERT_EQUAL(sets.SizeOfSet(0), i + 1);
            UNIT_ASSERT_EQUAL(sets.SetCount(), sets.InitialSetCount() - i);
        }

        TDisjointSets::TElement repr = sets.CanonicSetElement(0);
        for (size_t i = 0; i < sets.InitialSetCount(); ++i) {
            UNIT_ASSERT_EQUAL(sets.CanonicSetElement(i), repr);
            UNIT_ASSERT_EQUAL(sets.SizeOfSet(i), sets.InitialSetCount());
        }
    }

    Y_UNIT_TEST(ExpandTest) {
        TDisjointSets sets(0);
        sets.Expand(1000);
        UNIT_ASSERT_EQUAL(sets.SetCount(), 1000);
        UNIT_ASSERT_EQUAL(sets.InitialSetCount(), 1000);
    }
}
