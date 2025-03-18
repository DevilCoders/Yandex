#include "hashset.h"

#include <library/cpp/testing/unittest/registar.h>

class THashSetTest: public TTestBase {
private:
    UNIT_TEST_SUITE(THashSetTest);
    UNIT_TEST(TestHashSet1);
    UNIT_TEST(TestHashSet2);
    UNIT_TEST_SUITE_END();

protected:
    void TestHashSet1();
    void TestHashSet2();
};

UNIT_TEST_SUITE_REGISTRATION(THashSetTest);

void THashSetTest::TestHashSet1() {
    using H = THashSetType<int>;
    H h1;

    h1.Insert(100);
    h1.Insert(200);

    H h2(h1);

    UNIT_ASSERT_VALUES_EQUAL(h1.Size(), 2);
    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 2);
    UNIT_ASSERT(h1.Has(100));
    UNIT_ASSERT(h2.Has(200));

    H h3(std::move(h1));

    UNIT_ASSERT_VALUES_EQUAL(h1.Size(), 0);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 2);
    UNIT_ASSERT(h3.Has(100));

    h2.Insert(300);
    h3 = h2;

    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 3);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 3);
    UNIT_ASSERT(h3.Has(300));

    h2.Insert(400);
    h3 = std::move(h2);

    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 0);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 4);
    UNIT_ASSERT(h3.Has(400));
}

void THashSetTest::TestHashSet2() {
    using H = THashSetType<int>;
    H h;

    auto r = h.Insert(42);

    UNIT_ASSERT(r.second);
    UNIT_ASSERT(*(r.first) == 42);

    r = h.Insert(42);
    UNIT_ASSERT(!r.second);
}
