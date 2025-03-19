#include "any_iterator.h"
#include "filter_iterator.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/array_size.h>


Y_UNIT_TEST_SUITE(TestIterators) {

    void Iterate(TAnyIterator<int> begin, TAnyIterator<int> end, int values[], size_t size) {
        size_t i = 0;
        UNIT_ASSERT(begin != end);
        for(; begin != end; ++begin) {
            UNIT_ASSERT_EQUAL(values[i], *begin);
            ++i;
        }
        UNIT_ASSERT_EQUAL(size, i);
    }

    Y_UNIT_TEST(TestAnyIterator) {
        TVector<int> v{2, 3, 5};

        TAnyIterator<int> it1(v.begin());
        UNIT_ASSERT_VALUES_EQUAL(2, *it1);
        *it1 = 10;

        ++it1;
        UNIT_ASSERT_EQUAL(3, *it1);

        ++it1;
        UNIT_ASSERT_EQUAL(5, *it1);

        ++it1;
        UNIT_ASSERT_EQUAL(it1, TAnyIterator<int>(v.end()));
    }

    Y_UNIT_TEST(TestFilterIterator) {
        TVector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);

        int all[] = {1, 2, 3, 4};
        Iterate(v.begin(), v.end(), all, 4);

        auto filter = [](int v) { return v % 2 == 0; };
        TFilterIterator<TVector<int>::iterator, int, decltype(filter)> evenFilter(v.begin(), v.end(), filter);

        int evens[] = {2, 4};
        Iterate(evenFilter, evenFilter.End(), evens, 2);
    }
}
