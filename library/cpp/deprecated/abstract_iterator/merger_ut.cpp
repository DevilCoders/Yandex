#include <util/generic/vector.h>

#include <library/cpp/testing/unittest/registar.h>

#include "merger.h"

class TMergerTest: public TTestBase {
    UNIT_TEST_SUITE(TMergerTest);
    UNIT_TEST(TestMerger);
    UNIT_TEST_SUITE_END();

public:
    void TestMerger();
};

void TMergerTest::TestMerger() {
    typedef TVector<int> TIntVector;
    typedef TContIterator<TIntVector> TIntVectorIterator;
    TIntVector a = {1, 2, 4};
    TIntVector b = {-1, 1, 3};
    THolder<TIntVectorIterator> a_i(new TIntVectorIterator(a));
    THolder<TIntVectorIterator> b_i(new TIntVectorIterator(b));
    TVector<TIntVectorIterator*> its = {a_i.Get(), b_i.Get()};

    TMergerIterator<TIntVectorIterator> mit(its.begin(), its.end());
    TIntVector merged;
    Fill(mit, merged);

    TIntVector merged0 = {-1, 1, 1, 2, 3, 4};
    UNIT_ASSERT(merged == merged0);
}

UNIT_TEST_SUITE_REGISTRATION(TMergerTest);
