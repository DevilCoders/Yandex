#include "attrweightprop.h"

#include "metainfo.h"

#include <library/cpp/testing/unittest/registar.h>

class TAttrWeightPropagatorTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TAttrWeightPropagatorTest);
        UNIT_TEST(TestForward);
        UNIT_TEST(TestReverse);
        UNIT_TEST(TestIsEmpty);
        UNIT_TEST(TestNullAttr);
    UNIT_TEST_SUITE_END();

    THolder<NGroupingAttrs::TMetainfo> Meta;

public:
    void SetUp() override {
        // Category tree here looks as follows:
        //      9
        //    / | \      3
        //   7  1  5     |
        //  / \   / \    2
        // 10 4   6 8

        Meta.Reset(new NGroupingAttrs::TMetainfo(false));
        Meta->ScanStroka("7\t9\n1\t9\n5\t9\n10\t7\n4\t7\n6\t5\n8\t5\n2\t3\n", NGroupingAttrs::TMetainfo::C2P);
        UNIT_ASSERT_EQUAL(9, Meta->Categ2Parent(1));
    }

    void TestForward() {
        NGroupingAttrs::TAttrWeights w;
        w.push_back(NGroupingAttrs::TAttrWeight(9, 1.0f));
        w.push_back(NGroupingAttrs::TAttrWeight(7, 0.1f));
        w.push_back(NGroupingAttrs::TAttrWeight(2, 2.0f));
        w.push_back(NGroupingAttrs::TAttrWeight(5, 3.0f));
        w.push_back(NGroupingAttrs::TAttrWeight(100, 12345.0f));

        NGroupingAttrs::TAttrWeightPropagator p(false);
        p.Init(Meta.Get(), w, false);

        UNIT_ASSERT_EQUAL(1.0f, p.Get(1));
        UNIT_ASSERT_EQUAL(2.0f, p.Get(2));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(3));
        UNIT_ASSERT_EQUAL(1.0f, p.Get(4));
        UNIT_ASSERT_EQUAL(3.0f, p.Get(5));
        UNIT_ASSERT_EQUAL(3.0f, p.Get(6));
        UNIT_ASSERT_EQUAL(1.0f, p.Get(7));
        UNIT_ASSERT_EQUAL(3.0f, p.Get(8));
        UNIT_ASSERT_EQUAL(1.0f, p.Get(9));
        UNIT_ASSERT_EQUAL(1.0f, p.Get(10));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(20));
        UNIT_ASSERT_EQUAL(12345.0f, p.Get(100));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(-1));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(END_CATEG));
    }

    void TestReverse() {
        NGroupingAttrs::TAttrWeights w;
        w.push_back(NGroupingAttrs::TAttrWeight(9, 1.0f));
        w.push_back(NGroupingAttrs::TAttrWeight(7, 0.1f));
        w.push_back(NGroupingAttrs::TAttrWeight(2, 2.0f));
        w.push_back(NGroupingAttrs::TAttrWeight(5, 3.0f));
        w.push_back(NGroupingAttrs::TAttrWeight(100, 12345.0f));
        w.push_back(NGroupingAttrs::TAttrWeight(4, 0.3f));
        w.push_back(NGroupingAttrs::TAttrWeight(10, 0.4f));

        NGroupingAttrs::TAttrWeightPropagator p(false);
        /*p.Init(*AttrMap, "foo", w, true);*/
        p.Init(Meta.Get(), w, true);

        UNIT_ASSERT_EQUAL(0.0f, p.Get(1));
        UNIT_ASSERT_EQUAL(2.0f, p.Get(2));
        UNIT_ASSERT_EQUAL(2.0f, p.Get(3));
        UNIT_ASSERT_EQUAL(0.3f, p.Get(4));
        UNIT_ASSERT_EQUAL(3.0f, p.Get(5));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(6));
        UNIT_ASSERT_EQUAL(0.4f, p.Get(7));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(8));
        UNIT_ASSERT_EQUAL(3.0f, p.Get(9));
        UNIT_ASSERT_EQUAL(0.4f, p.Get(10));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(20));
        UNIT_ASSERT_EQUAL(12345.0f, p.Get(100));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(-1));
        UNIT_ASSERT_EQUAL(0.0f, p.Get(END_CATEG));
    }

    void TestIsEmpty() {
        NGroupingAttrs::TAttrWeightPropagator p(false);

        p.Init(Meta.Get(), 1, 123.0f);
        UNIT_ASSERT(!p.IsEmpty());

        p.Init(Meta.Get(), NGroupingAttrs::TAttrWeights(), false);
        UNIT_ASSERT(p.IsEmpty());

        p.Init(Meta.Get(), 1, 0.0f);
        UNIT_ASSERT(!p.IsEmpty());

        p.Init(Meta.Get(), END_CATEG, 123.0f);
        UNIT_ASSERT(p.IsEmpty());
    }

    void TestNullAttr() {
        // NGroupingAttrs::TAttrWeightPropagator should tolerate non-existent attributes
        NGroupingAttrs::TAttrWeightPropagator p(false);
        p.Init(nullptr, 12, 34.0f);
        UNIT_ASSERT_EQUAL(0.0f, p.Get(1));
        UNIT_ASSERT_EQUAL(34.0f, p.Get(12));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TAttrWeightPropagatorTest);
