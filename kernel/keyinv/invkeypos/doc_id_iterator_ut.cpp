#include "doc_id_iterator.h"

#include <library/cpp/testing/unittest/registar.h>


class TDocIdIteratorTest: public TTestBase
{
    UNIT_TEST_SUITE(TDocIdIteratorTest);
        UNIT_TEST(TestSimple);
        UNIT_TEST(TestMergeOrder);
        UNIT_TEST(TestMergePosition);
    UNIT_TEST_SUITE_END();
public:
    void TestSimple();
    void TestMergeOrder();
    void TestMergePosition();
};


void TDocIdIteratorTest::TestSimple() {
    TDocIdIterator it;
    it.Add(1);
    it.Add(2);
    it.Add(3);
    it.Add(5);
    it.Add(7);
    UNIT_ASSERT_VALUES_EQUAL(1u, it.PeekDoc());
    UNIT_ASSERT_VALUES_EQUAL((3 << DOC_LEVEL_Shift), it.Next(3 << DOC_LEVEL_Shift));
    UNIT_ASSERT_VALUES_EQUAL((7 << DOC_LEVEL_Shift), it.Next(6 << DOC_LEVEL_Shift));
    UNIT_ASSERT_VALUES_EQUAL(7, it.PeekDoc());
    it.Add(8);
    it.Advance();
    UNIT_ASSERT_VALUES_EQUAL(8, it.PeekDoc());
    it.Advance();
    UNIT_ASSERT(it.Eof());
    it.Add(9);
    UNIT_ASSERT_VALUES_EQUAL(9, it.PeekDoc());
    it.Advance();
    UNIT_ASSERT(it.Eof());
}

void TDocIdIteratorTest::TestMergeOrder() {
    TVector<ui32> initDocs = {1, 5, 10, 15};
    TDocIdIterator it(initDocs.begin(), initDocs.end());
    TVector<ui32> newDocs = {5, 15, 7, 20};
    it.MergeDocs(newDocs.begin(), newDocs.end());

    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 1);
    it.Advance();
    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 5);
    it.Advance();
    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 7);
    it.Advance();
    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 10);
    it.Advance();
    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 15);
    it.Advance();
    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 20);
    it.Advance();
    UNIT_ASSERT(it.Eof());
}

void TDocIdIteratorTest::TestMergePosition() {
    TVector<ui32> initDocs = {1, 5, 10, 15};
    TDocIdIterator it(initDocs.begin(), initDocs.end());
    it.Advance(); /* now points to 5 and we don't want to see 1 later at any point */

    TVector<ui32> newDocs = {3, 5, 7};
    it.MergeDocs(newDocs.begin(), newDocs.end());
    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 3);

    while (!it.Eof()) {
        it.Advance();
    }
    /* it points to eof (docs <= 15 are not relevant now) */
    TVector<ui32> newDocs1 = {15, 17, 19, 20};
    it.MergeDocs(newDocs1.begin(), newDocs1.end());
    UNIT_ASSERT_VALUES_EQUAL(it.PeekDoc(), 17);
}


UNIT_TEST_SUITE_REGISTRATION(TDocIdIteratorTest);
