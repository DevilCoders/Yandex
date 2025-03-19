#include <library/cpp/testing/unittest/registar.h>

#include <kernel/segnumerator/segment.h>

#include <kernel/segmentator/structs/merge.h>

class TSegmentatorTest: public TTestBase {
UNIT_TEST_SUITE(TSegmentatorTest);
        UNIT_TEST(SumDiffTest);
        UNIT_TEST(AddNodeTest);
        UNIT_TEST(PeriodicMergeTest);
        UNIT_TEST(SegmentSpanTest);
        UNIT_TEST_SUITE_END();

private:
    TMemoryPool Pool;

public:
    TSegmentatorTest()
        : Pool(4000)
    {}

private:
    void TestDiff(NSegm::TBlockDist reference, NSegm::TBlockDist value,
            const ui32 comm, const TString& code) {
        TString comment = ToString(comm) + " " + code;
        UNIT_ASSERT_VALUES_EQUAL_C(reference.SuperItems, value.SuperItems, comment.c_str());
        UNIT_ASSERT_VALUES_EQUAL_C(reference.Items, value.Items, comment.c_str());
        UNIT_ASSERT_VALUES_EQUAL_C(reference.Paragraphs, value.Paragraphs, comment.c_str());
        UNIT_ASSERT_VALUES_EQUAL_C(reference.Blocks, value.Blocks, comment.c_str());
    }

    void TestSumDiff(NSegm::NPrivate::TDocNode * n1
                     , NSegm::NPrivate::TDocNode * n2
                     , NSegm::TBlockDist ttdist
                     , NSegm::TBlockDist tfdist
                     , NSegm::TBlockDist ftdist
                     , NSegm::TBlockDist ffdist
                     , ui32 comm) {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        {
            TBlockInfo b1 = TBlockInfo::Make(true);
            TBlockInfo b2 = TBlockInfo::Make(true);
            TestDiff(ttdist, SumDist(b1, b2, n1, n2), comm, "tt");
        }
        {
            TBlockInfo b1 = TBlockInfo::Make(true);
            TBlockInfo b2 = TBlockInfo::Make(false);
            TestDiff(tfdist, SumDist(b1, b2, n1, n2), comm, "tf");
        }
        {
            TBlockInfo b1 = TBlockInfo::Make(false);
            TBlockInfo b2 = TBlockInfo::Make(true);
            TestDiff(ftdist, SumDist(b1, b2, n1, n2), comm, "ft");
        }
        {
            TBlockInfo b1 = TBlockInfo::Make(false);
            TBlockInfo b2 = TBlockInfo::Make(false);
            TestDiff(ffdist, SumDist(b1, b2, n1, n2), comm, "ff");
        }
    }

    void SumDiffTest() {
        using namespace NSegm::NPrivate;
        using NSegm::TBlockDist;
        /*
         root <- child01=table <- child11=tr <- child21=td <- child31=div
         .....^- child02=ul    <- child12=li <- child22=div
         ......................^- child13=li
         .....^- child03=p
         .....^- child04=div
         .....^- child05=div   <- child15=div
         .....^- child06=div   <- child16=div
         */
        TDocNode* root = MakeBlock(Pool, 0, nullptr, HT_any);
        TDocNode* child01 = MakeBlock(Pool, 0, root, HT_TABLE);
        TDocNode* child11 = MakeBlock(Pool, 0, child01, HT_TR);
        TDocNode* child21 = MakeBlock(Pool, 0, child11, HT_TD);
        TDocNode* child31 = MakeBlock(Pool, 0, child21, HT_DIV);

        TDocNode* child02 = MakeBlock(Pool, 0, root, HT_UL);
        TDocNode* child12 = MakeBlock(Pool, 0, child02, HT_LI);
        TDocNode* child13 = MakeBlock(Pool, 0, child02, HT_LI);
        TDocNode* child22 = MakeBlock(Pool, 0, child12, HT_DIV);

        TDocNode* child03 = MakeBlock(Pool, 0, root, HT_P);
        TDocNode* child04 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode* child05 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode* child15 = MakeBlock(Pool, 0, child05, HT_DIV);
        TDocNode* child06 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode* child16 = MakeBlock(Pool, 0, child06, HT_DIV);

        ui32 i = 0;
        TestSumDiff(child01, child01, TBlockDist::Make(), TBlockDist::Make(1, 0, 0),
                TBlockDist::Make(1, 0, 0), TBlockDist::Make(), i++);
        TestSumDiff(child02, child02, TBlockDist::Make(), TBlockDist::Make(0, 1, 0),
                TBlockDist::Make(0, 1, 0), TBlockDist::Make(), i++);
        TestSumDiff(child03, child03, TBlockDist::Make(), TBlockDist::Make(0, 1, 0),
                TBlockDist::Make(0, 1, 0), TBlockDist::Make(), i++);
        TestSumDiff(child04, child04, TBlockDist::Make(), TBlockDist::Make(1, 0, 0),
                TBlockDist::Make(1, 0, 0), TBlockDist::Make(), i++);

        TestSumDiff(child11, child11, TBlockDist::Make(), TBlockDist::Make(0, 0, 1),
                TBlockDist::Make(0, 0, 1), TBlockDist::Make(), i++);
        TestSumDiff(child12, child12, TBlockDist::Make(), TBlockDist::Make(0, 0, 1),
                TBlockDist::Make(0, 0, 1), TBlockDist::Make(), i++);
        TestSumDiff(child12, child13, TBlockDist::Make(0, 0, 2), TBlockDist::Make(0, 0, 1),
                TBlockDist::Make(0, 0, 1), TBlockDist::Make(), i++);
        TestSumDiff(child21, child21, TBlockDist::Make(), TBlockDist::Make(), TBlockDist::Make(),
                TBlockDist::Make(), i++);
        TestSumDiff(child31, child31, TBlockDist::Make(), TBlockDist::Make(1, 0, 0),
                TBlockDist::Make(1, 0, 0), TBlockDist::Make(), i++);

        TestSumDiff(child31, child01, TBlockDist::Make(1, 0, 1), TBlockDist::Make(2, 0, 1),
                TBlockDist::Make(0, 0, 1), TBlockDist::Make(1, 0, 1), i++);
        TestSumDiff(child01, child31, TBlockDist::Make(1, 0, 1), TBlockDist::Make(0, 0, 1),
                TBlockDist::Make(2, 0, 1), TBlockDist::Make(1, 0, 1), i++);
        TestSumDiff(child22, child02, TBlockDist::Make(1, 0, 1), TBlockDist::Make(1, 1, 1),
                TBlockDist::Make(0, 0, 1), TBlockDist::Make(0, 1, 1), i++);
        TestSumDiff(child15, child16, TBlockDist::Make(4, 0, 0), TBlockDist::Make(3, 0, 0),
                TBlockDist::Make(3, 0, 0), TBlockDist::Make(2, 0, 0), i++);
    }

    void AddNodeTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        //todo: implement and test header detection

        TDocNode * text1 = MakeText(Pool, 0, 1, nullptr, 2);
        TDocNode * text2 = MakeText(Pool, 0, 1, nullptr, 3);
        TDocNode * br = MakeBreak(Pool, 0, nullptr, TBL_BR);
        TDocNode * tr = MakeBreak(Pool, 0, nullptr, TBL_TR);
        TDocNode * inp1 = MakeInput(Pool, 0, nullptr, 2);
        TDocNode * inp2 = MakeInput(Pool, 0, nullptr, 1);

        TDocNode * root = MakeBlock(Pool, 0, nullptr, HT_any);
        TDocNode * anc = MakeLink(Pool, 0, root, LT_LOCAL_LINK, 0);

        anc->PushBack(text1);
        anc->PushBack(text2);
        anc->PushBack(br);
        anc->PushBack(tr);
        anc->PushBack(inp1);
        anc->PushBack(inp2);

        TDocNode * ancint = MakeLink(Pool, 0, root, LT_LOCAL_LINK, 0);
        TDocNode * ancext1 = MakeLink(Pool, 0, root, LT_EXTERNAL_LINK, 1);
        TDocNode * ancext2 = MakeLink(Pool, 0, root, LT_EXTERNAL_LINK, 2);
        TDocNode * ancext3 = MakeLink(Pool, 0, root, LT_EXTERNAL_LINK, 3);

        TSegmentNode node;

        node.Add(text1);
        node.Add(text2);
        node.Add(tr);
        node.Add(tr);
        node.Add(inp1);
        node.Add(inp2);

        UNIT_ASSERT(5 == node.Stats.Words);
        UNIT_ASSERT(0 == node.Stats.LinkWords);
        UNIT_ASSERT(2 == node.Stats.Breaks);
        UNIT_ASSERT(3 == node.Stats.Inputs);

        node.Add(anc);

        UNIT_ASSERT(1 == node.Stats.Links);
        UNIT_ASSERT(0 == node.Stats.Domains());
        UNIT_ASSERT(1 == node.Stats.LocalLinks);

        UNIT_ASSERT(10 == node.Stats.Words);
        UNIT_ASSERT(5 == node.Stats.LinkWords);
        UNIT_ASSERT(3 == node.Stats.Breaks);
        UNIT_ASSERT(6 == node.Stats.Inputs);

        node.Add(ancext1);

        UNIT_ASSERT(2 == node.Stats.Links);
        UNIT_ASSERT(1 == node.Stats.Domains());
        UNIT_ASSERT(1 == node.Stats.LocalLinks);

        node.Add(ancint);

        UNIT_ASSERT(3 == node.Stats.Links);
        UNIT_ASSERT(1 == node.Stats.Domains());
        UNIT_ASSERT(2 == node.Stats.LocalLinks);

        node.Add(ancext2);

        UNIT_ASSERT(4 == node.Stats.Links);
        UNIT_ASSERT(2 == node.Stats.Domains());
        UNIT_ASSERT(2 == node.Stats.LocalLinks);

        node.Add(ancext1);

        UNIT_ASSERT(5 == node.Stats.Links);
        UNIT_ASSERT(2 == node.Stats.Domains());
        UNIT_ASSERT(2 == node.Stats.LocalLinks);

        node.Add(ancext3);

        UNIT_ASSERT(6 == node.Stats.Links);
        UNIT_ASSERT(3 == node.Stats.Domains());
        UNIT_ASSERT(2 == node.Stats.LocalLinks);

        node.Add(ancext3);

        UNIT_ASSERT(7 == node.Stats.Links);
        UNIT_ASSERT(3 == node.Stats.Domains());
        UNIT_ASSERT(2 == node.Stats.LocalLinks);

    }

    void SetBlocks(NSegm::NPrivate::TSegmentNode& s,
                   NSegm::NPrivate::TDocNode* a, bool ai, NSegm::NPrivate::TDocNode* b, bool bi) {
        s.SetFrontBlock(a, ai);
        s.SetBackBlock(b, bi);
    }

    void PeriodicMergeTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;

        TDocNode * b[12];
        b[0] = MakeBlock(Pool, 0, nullptr, HT_any);

        TAlignedPosting p;

        for (int i = 1; i < 12; i++) {
            p.Set(i, 1);
            HT_TAG t = (HT_TAG)(i < 8 ? HT_H1 + i % 3 : HT_H1);
            b[i] = MakeBlock(Pool, p, b[0], t);
            b[i - 1]->NodeEnd = p;
        }

        b[11]->NodeEnd = p;
        b[0]->NodeEnd = p;

        /* 0  1  2  3  4  5  6  7  8  9 10 11
         * 0
         *   h2 h3 h1 h2 h3 h1 h2 h1 h1 h1 h1+
         *   h2 h3 h1 h2 h3 X
         */

        TSegmentNode s[12];

        for (int i = 0; i < 11; ++i)
            SetBlocks(s[i], b[i], true, b[i + 1], false);

        SetBlocks(s[11], b[11], true, b[11], true);

        TSegmentList list;

        for (int i = 0; i < 11; i++)
            list.PushBack(&s[i]);

        TSegmentSpans lst;
        MakeSegmentList(lst, list);

        UNIT_ASSERT(EqualSignatures(lst[1], lst[4]));
        UNIT_ASSERT(!EqualSignatures(lst[0], lst[1]));
        UNIT_ASSERT(!EqualSignatures(lst[1], lst[2]));
        UNIT_ASSERT(!EqualSignatures(lst[1], lst[3]));
        UNIT_ASSERT(EqualSignatures(lst[6], lst[8]));
        UNIT_ASSERT(EqualSignatures(lst[8], lst[9]));
        UNIT_ASSERT_VALUES_EQUAL(GetMaxPeriodicLen(lst.begin() + 1, lst.begin() + 8), 1u);
        UNIT_ASSERT_VALUES_EQUAL(GetMaxPeriodicLen(lst.begin() + 8, lst.begin() + 9), 1u);
        UNIT_ASSERT_VALUES_EQUAL(GetMaxPeriodicLen(lst.begin() + 8, lst.begin() + 10), 2u);
        UNIT_ASSERT_VALUES_EQUAL(GetMaxPeriodicLen(lst.begin() + 8, lst.end()), 3u);

        PeriodicMerge<TSegSpanVecAccessor> (lst);
        UNIT_ASSERT_VALUES_EQUAL(9u, lst.size());
    }

    void SegmentSpanTest() {
        using namespace NSegm;
        TSegmentSpan sp;
        TSegmentSpan sp2;

        sp.AllMarkers = 0xff;
        sp.AllMarkers2 = 3;

        sp2.SetMarkers(sp.GetMarkers());

        UNIT_ASSERT_VALUES_EQUAL(sp2.AllMarkers, 0xff);
        UNIT_ASSERT_VALUES_EQUAL(sp2.AllMarkers2, 3);
    }
};
UNIT_TEST_SUITE_REGISTRATION(TSegmentatorTest)
;
