#include "container.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/system/type_name.h>

Y_UNIT_TEST_SUITE(AlgorithmSuite) {
    Y_UNIT_TEST(Intersection) {
        TVector<ui8> first = { 0, 1, 2, 3 };
        TSet<ui16> second = { 1042, 2, 42 };
        TSet<ui32> third = { 0, 3, 42 };
        TVector<ui64> forth = { 43, 44, 45 };

        UNIT_ASSERT(!IsIntersectionEmpty(first, second));
        UNIT_ASSERT(!IsIntersectionEmpty(second, first));
        UNIT_ASSERT(!IsIntersectionEmpty(first, third));
        UNIT_ASSERT(!IsIntersectionEmpty(third, first));
        UNIT_ASSERT(IsIntersectionEmpty(first, forth));
        UNIT_ASSERT(IsIntersectionEmpty(forth, first));
        UNIT_ASSERT(!IsIntersectionEmpty(second, third));
        UNIT_ASSERT(IsIntersectionEmpty(forth, second));

        /*
        TMap<ui32, TString> one = { { 1, "one" }, { 2, "two" } };
        TMap<ui32, TString> two = { { 1, "one"}, { 2, "forty-two" } };
        TMap<ui32, TString> three = { { 1, "forty-one"}, { 2, "forty-two" } };
        UNIT_ASSERT(!IsIntersectionEmpty(one, two));
        UNIT_ASSERT(!IsIntersectionEmpty(two, one));
        UNIT_ASSERT(IsIntersectionEmpty(one, three));
        UNIT_ASSERT(IsIntersectionEmpty(two, three));
        */
    }

    Y_UNIT_TEST(Container) {
        TMap<ui32, TString> container = { { 1, "one" }, { 2, "two" } };
        for (auto&& key : NContainer::Keys(container)) {
            Cerr << key << Endl;
        }
        for (auto&& value : NContainer::Values(container)) {
            Cerr << value << Endl;
        }
        {
            TVector<ui32> keys = MakeVector(NContainer::Keys(container));
            TVector<TString> values = MakeVector(NContainer::Values(container));
            UNIT_ASSERT_VALUES_EQUAL(keys.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(values.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(keys[0], 1);
            UNIT_ASSERT_VALUES_EQUAL(keys[1], 2);
            UNIT_ASSERT_VALUES_EQUAL(values[0], "one");
            UNIT_ASSERT_VALUES_EQUAL(values[1], "two");
        }
        {
            TSet<ui32> keys = MakeSet(NContainer::Keys(container));
            TSet<TString> values = MakeSet(NContainer::Values(container));
            UNIT_ASSERT_VALUES_EQUAL(keys.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(values.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(*keys.begin(), 1);
            UNIT_ASSERT_VALUES_EQUAL(*values.begin(), "one");
        }
        {
            TVector<double> doubleKeys = MakeVector<double>(NContainer::Keys(container));
            TSet<float> floatKeys = MakeSet<float>(NContainer::Keys(container));
        }
    }
}
