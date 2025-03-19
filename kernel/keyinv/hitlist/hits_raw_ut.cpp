#include <util/stream/output.h>

#include <library/cpp/testing/unittest/registar.h>

#include <kernel/keyinv/hitlist/hits_raw.h>

class TRawHitsTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TRawHitsTest);
        UNIT_TEST(TestEmpty);
        UNIT_TEST(TestSimple);
        UNIT_TEST(TestSingle);
    UNIT_TEST_SUITE_END();

public:
    void TestEmpty();
    void TestSimple();
    void TestSingle();
};

UNIT_TEST_SUITE_REGISTRATION(TRawHitsTest);

void TRawHitsTest::TestEmpty()
{
    TRawHits hits;
    TRawHitsIter iter;
    iter.Init(hits);
    UNIT_ASSERT(!iter.Valid());
}

void TRawHitsTest::TestSimple()
{
    TRawHits hits;
    hits.PushBack(1);
    hits.PushBack(5);
    hits.PushBack(50);
    TRawHitsIter iter;
    iter.Init(hits);
    UNIT_ASSERT_EQUAL(*iter, 1); ++iter;
    UNIT_ASSERT_EQUAL(*iter, 5); ++iter;
    UNIT_ASSERT_EQUAL(*iter, 50); ++iter;
    UNIT_ASSERT(!iter.Valid());
}

void TRawHitsTest::TestSingle()
{
    TRawHits hits;
    hits.PushBack(1);
    TRawHitsIter iter;
    iter.Init(hits);
    UNIT_ASSERT(iter.Valid());
    UNIT_ASSERT_EQUAL(*iter, 1); ++iter;
    UNIT_ASSERT(!iter.Valid());
}
