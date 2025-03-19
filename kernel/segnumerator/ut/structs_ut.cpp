#include <library/cpp/testing/unittest/registar.h>

#include <kernel/segnumerator/doc_node.h>

class TTreeBuilderUtilsTest: public TTestBase {
UNIT_TEST_SUITE(TTreeBuilderUtilsTest);
        UNIT_TEST(OwnerInfoTest);
        UNIT_TEST(PrevNextTest);
        UNIT_TEST(EraseTest);
        UNIT_TEST(IsTelescopicTest);
        UNIT_TEST(IsCollapseableTest);
        UNIT_TEST(IsReplaceableTest);
        UNIT_TEST(ListTest);
        UNIT_TEST(ReplaceTest);
        UNIT_TEST(DetelescopeTest);
        UNIT_TEST(NodesEqualTest);
        UNIT_TEST_SUITE_END();
private:
    struct TItem: TIntrusiveListItem<TItem> {
        ui32 I = 0;
    };

    TMemoryPool Pool;
public:
    TTreeBuilderUtilsTest()
        : Pool(100000)
    {}
private:
    void OwnerInfoTest() {
        using namespace NSegm;
        TOwnerCanonizer can;
        can.AddDom2("narod.su");
        can.AddDom2("com.ua");
        can.AddDom2("gov.ua");

        {
            TUrlInfo info("http://www.peter.narod.su/test.html?", &can);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("peter.narod.su:8080;ttt"), (int)LT_LOCAL_LINK);
        }

        {
            TUrlInfo info("http://gov.ua/", &can);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("http://i.gov.ua/"), (int)LT_EXTERNAL_LINK);
        }

        {
            TUrlInfo info("http://k.i.gov.ua/", &can);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("http://gov.ua"), (int)LT_LOCAL_LINK);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("http://j.gov.ua"), (int)LT_EXTERNAL_LINK);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("http://j.i.gov.ua"), (int)LT_LOCAL_LINK);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("http://www.k.i.gov.ua"), (int)LT_LOCAL_LINK);
        }
        {
            TUrlInfo info("http://www.coolershop.com.ua/ru/catalogue/floor/floor-download.html", &can);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("http://www.coolershop.com.ua/ru/servises/guarantee.html"),
                                     (int)LT_LOCAL_LINK);
        }
        {
            TUrlInfo info("http://forum.lki.ru/index.php?do=new_post&f=28&Article=3718&Title=Alan+Wake", &can);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("http://forum.lki.ru/index.php?Article=3718&Do=new_post&f=28&Title=Alan%20Wake"),
                                     (int)LT_FRAGMENT_LINK);
        }
        {
            TUrlInfo info("http://yandex.ru", &can);
            UNIT_ASSERT_VALUES_EQUAL((int)info.CheckLink("javascript:test();"), (int)LT_LOCAL_LINK);
        }

    }

    void FillList(TIntrusiveList<TItem>& list, TItem * items, ui32 sz) {
        for (ui32 i = 0; i < sz; ++i) {
            items[i].I = i;
            list.PushBack(items + i);
        }
    }

    void PrevNextTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;

        TItem items[10];
        TIntrusiveList<TItem> list;

        FillList(list, items, 10);

        ui32 i = 0;

        for (TIntrusiveList<TItem>::iterator it = list.Begin(); it != list.End(); ++it, ++i) {
            TItem * prevprev = TListAccessor<TItem, TIntrusiveList<TItem> >::GetPrevPrev(it, list.Begin());
            TItem * prev = TListAccessor<TItem, TIntrusiveList<TItem> >::GetPrev(it, list.Begin());
            TItem * next = TListAccessor<TItem, TIntrusiveList<TItem> >::GetNext(it, list.End());
            TItem * nextnext = TListAccessor<TItem, TIntrusiveList<TItem> >::GetNextNext(it, list.End());

            UNIT_ASSERT(0 == i && nullptr == prev || 0 != i && &items[i-1] == prev);
            UNIT_ASSERT(9 == i && nullptr == next || 9 != i && &items[i+1] == next);
            UNIT_ASSERT(0 == i || 1 == i && nullptr == prevprev || 0 != i && 1 != i && &items[i-2] == prevprev);
            UNIT_ASSERT(9 == i || 8 == i && nullptr == nextnext || 9 != i && 8 != i && &items[i+2] == nextnext);
        }
    }

    static TIntrusiveList<TItem>::iterator DoErase(TIntrusiveList<TItem>::iterator it) {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        it = TListAccessor<TItem, TIntrusiveList<TItem> >::Erase(it);
        return --it;
    }

    static TIntrusiveList<TItem>::iterator DoEraseBack(TIntrusiveList<TItem>::iterator it) {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        return TListAccessor<TItem, TIntrusiveList<TItem> >::EraseBack(it);
    }

    template<TIntrusiveList<TItem>::iterator (*Act)(TIntrusiveList<TItem>::iterator)>
    void TestErase() {
        TItem items[10];
        TIntrusiveList<TItem> list;

        FillList(list, items, 10);

        for (TIntrusiveList<TItem>::iterator it = list.Begin(); it != list.End(); ++it)
            if (!(it->I % 2))
                it = Act(it);

        UNIT_ASSERT(5 == list.Size());

        ui32 i = 1;
        for (TIntrusiveList<TItem>::iterator it = list.Begin(); it != list.End(); ++it, i += 2)
            UNIT_ASSERT(i == it->I);
    }

    void EraseTest() {
        TestErase<DoErase>();
        TestErase<DoEraseBack>();
    }

    void IsTelescopicTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        /*
         root <- ch11 <- ch21 <- ch31:b
         .....^- ch12 <- ch22 <- ch32:l
         .....^- ch13 <- ch23 <- ch33:b
         .....^- ch34:b
         */
        TDocNode * root = MakeBlock(Pool, 0, nullptr, HT_any);

        TDocNode * ch11 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch21 = MakeBlock(Pool, 0, ch11, HT_DIV);
        TDocNode * ch31 = MakeBlock(Pool, 0, ch21, HT_DIV);

        TDocNode * ch12 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch22 = MakeBlock(Pool, 0, ch12, HT_DIV);
        TDocNode * ch32 = MakeLink(Pool, 0, ch22, LT_LOCAL_LINK, 0);

        TDocNode * ch13 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch23 = MakeBlock(Pool, 0, ch13, HT_DIV);
        TDocNode * ch33 = MakeBlock(Pool, 0, ch23, HT_DIV);
        TDocNode * ch34 = MakeBlock(Pool, 0, ch23, HT_DIV);

        UNIT_ASSERT(!MakeBlock(Pool, 0, nullptr, HT_any)->IsTelescopic());
        UNIT_ASSERT(!root->IsTelescopic());
        UNIT_ASSERT(ch11->IsTelescopic());
        UNIT_ASSERT(ch21->IsTelescopic());
        UNIT_ASSERT(!ch31->IsTelescopic());
        UNIT_ASSERT(ch12->IsTelescopic());
        UNIT_ASSERT(!ch22->IsTelescopic());
        UNIT_ASSERT(!ch32->IsTelescopic());
        UNIT_ASSERT(ch13->IsTelescopic());
        UNIT_ASSERT(!ch23->IsTelescopic());
        UNIT_ASSERT(!ch33->IsTelescopic());
        UNIT_ASSERT(!ch34->IsTelescopic());
    }

    void IsCollapseableTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        /*
         root <- ch11:div <- ch21:div
         .....^- ch12:div <- ch22:table <- ch32:tr <- ch42:td <- ch52:div
         .....^- ch13:div <- ch23:ul    <- ch33:li <- ch43:div
         .....^- ch14:div <- ch24:h1    <- ch34:ul <- ch44:li
         */
        TDocNode * root = MakeBlock(Pool, 0, nullptr, HT_any);

        TDocNode * ch11 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch21 = MakeBlock(Pool, 0, ch11, HT_DIV);

        TDocNode * ch12 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch22 = MakeBlock(Pool, 0, ch12, HT_TABLE);
        TDocNode * ch32 = MakeBlock(Pool, 0, ch22, HT_TR);
        TDocNode * ch42 = MakeBlock(Pool, 0, ch32, HT_TD);
        TDocNode * ch52 = MakeBlock(Pool, 0, ch42, HT_DIV);

        TDocNode * ch13 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch23 = MakeBlock(Pool, 0, ch13, HT_UL);
        TDocNode * ch33 = MakeBlock(Pool, 0, ch23, HT_LI);
        TDocNode * ch43 = MakeBlock(Pool, 0, ch33, HT_DIV);

        TDocNode * ch14 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch24 = MakeBlock(Pool, 0, ch14, HT_H1);
        TDocNode * ch34 = MakeBlock(Pool, 0, ch24, HT_UL);
        TDocNode * ch44 = MakeBlock(Pool, 0, ch34, HT_LI);

        UNIT_ASSERT(!root->IsCollapseable());
        UNIT_ASSERT(ch11->IsCollapseable());
        UNIT_ASSERT(!ch21->IsCollapseable());
        UNIT_ASSERT(!ch12->IsCollapseable());
        UNIT_ASSERT(!ch22->IsCollapseable());
        UNIT_ASSERT(!ch32->IsCollapseable());
        UNIT_ASSERT(!ch42->IsCollapseable());
        UNIT_ASSERT(!ch52->IsCollapseable());
        UNIT_ASSERT(!ch13->IsCollapseable());
        UNIT_ASSERT(!ch23->IsCollapseable());
        UNIT_ASSERT(!ch33->IsCollapseable());
        UNIT_ASSERT(!ch43->IsCollapseable());
        UNIT_ASSERT(ch14->IsCollapseable());
        UNIT_ASSERT(!ch24->IsCollapseable());
        UNIT_ASSERT(!ch34->IsCollapseable());
        UNIT_ASSERT(!ch44->IsCollapseable());
    }

    void IsReplaceableTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        /*
         root <- ch11:div <- ch21:div
         .....^- ch12:div <- ch22:table <- ch32:tr <- ch42:td <- ch52:div
         .....^- ch13:div <- ch23:ul    <- ch33:li <- ch43:div
         .....^- ch14:div <- ch24:h1    <- ch34:ul <- ch44:li
         */
        TDocNode * root = MakeBlock(Pool, 0, nullptr, HT_any);

        TDocNode * ch11 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch21 = MakeBlock(Pool, 0, ch11, HT_DIV);

        TDocNode * ch12 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch22 = MakeBlock(Pool, 0, ch12, HT_TABLE);
        TDocNode * ch32 = MakeBlock(Pool, 0, ch22, HT_TR);
        TDocNode * ch42 = MakeBlock(Pool, 0, ch32, HT_TD);
        TDocNode * ch52 = MakeBlock(Pool, 0, ch42, HT_DIV);

        TDocNode * ch13 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch23 = MakeBlock(Pool, 0, ch13, HT_UL);
        TDocNode * ch33 = MakeBlock(Pool, 0, ch23, HT_LI);
        TDocNode * ch43 = MakeBlock(Pool, 0, ch33, HT_DIV);

        TDocNode * ch14 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode * ch24 = MakeBlock(Pool, 0, ch14, HT_H1);
        TDocNode * ch34 = MakeBlock(Pool, 0, ch24, HT_UL);
        TDocNode * ch44 = MakeBlock(Pool, 0, ch34, HT_LI);

        UNIT_ASSERT(!root->IsReplaceable());
        UNIT_ASSERT(!ch11->IsReplaceable());
        UNIT_ASSERT(!ch21->IsReplaceable());
        UNIT_ASSERT(!ch12->IsReplaceable());
        UNIT_ASSERT(ch22->IsReplaceable());
        UNIT_ASSERT(!ch32->IsReplaceable());
        UNIT_ASSERT(!ch42->IsReplaceable());
        UNIT_ASSERT(!ch52->IsReplaceable());
        UNIT_ASSERT(!ch13->IsReplaceable());
        UNIT_ASSERT(ch23->IsReplaceable());
        UNIT_ASSERT(!ch33->IsReplaceable());
        UNIT_ASSERT(!ch43->IsReplaceable());
        UNIT_ASSERT(!ch14->IsReplaceable());
        UNIT_ASSERT(!ch24->IsReplaceable());
        UNIT_ASSERT(ch34->IsReplaceable());
        UNIT_ASSERT(!ch44->IsReplaceable());
    }

    void ListTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;

        TDocNode * root = MakeBlock(Pool, 0, nullptr, HT_any);
        const TDocNode * ch[] = { MakeBlock(Pool, 0, root, HT_P), MakeBlock(Pool, 0, root, HT_P), MakeBlock(Pool, 0,
                root, HT_P) };

        UNIT_ASSERT(ch[0] == root->Front());
        UNIT_ASSERT(ch[2] == root->Back());
        UNIT_ASSERT(ch[0] == &*(root->Begin()));
        UNIT_ASSERT(ch[2] == &*(--root->End()));
        UNIT_ASSERT(ch[2] == &*(root->RBegin()));
        UNIT_ASSERT(ch[0] == &*(--root->REnd()));

        {
            TDocNode::iterator it = root->Begin();
            ++it;
            UNIT_ASSERT(ch[1] == &*it);
            UNIT_ASSERT(ch[0] == TNodeAccessor::GetPrev(it, root->Begin()));
            UNIT_ASSERT(ch[2] == TNodeAccessor::GetNext(it, root->End()));
        }
        {
            TDocNode::reverse_iterator it = root->RBegin();
            ++it;
            UNIT_ASSERT(ch[1] == &*it);
            UNIT_ASSERT(ch[2] == TNodeAccessor::GetPrev(it, root->RBegin()));
            UNIT_ASSERT(ch[0] == TNodeAccessor::GetNext(it, root->REnd()));
        }

        {
            ui32 i = 0;
            for (TDocNode::iterator it = root->Begin(); it != root->End(); ++it, ++i)
                UNIT_ASSERT(ch[i] == &*it);
        }
        {
            ui32 i = 2;
            for (TDocNode::reverse_iterator it = root->RBegin(); it != root->End(); ++it, --i)
                UNIT_ASSERT(ch[i] == &*it);
        }
    }

    void NodesEqualTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        /*
         root <- child01 <- child11
         ^- child02 <- child12
         ^- child03 <- child13
         ^- child04 <- child14
         ^- child05 <- child15
         */
        TDocNode* root = MakeBlock(Pool, 0, nullptr, HT_any);
        TDocNode* child01 = MakeBlock(Pool, 0, root, HT_H1);
        TDocNode* child11 = MakeBlock(Pool, 0, child01, HT_DIV);

        child11->Props.Class = 3;
        child11->Props.Width = 5;
        child11->GenerateSignature();

        TDocNode* child02 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode* child12 = MakeBlock(Pool, 0, child02, HT_DIV);

        child02->Props.Class = 1;
        child02->GenerateSignature();
        child12->Props.Class = 3;
        child12->Props.Width = 5;
        child12->GenerateSignature();

        TDocNode* child03 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode* child13 = MakeBlock(Pool, 0, child03, HT_DIV);

        child03->Props.Width = 1;
        child03->GenerateSignature();
        child13->Props.Class = 3;
        child13->Props.Width = 5;
        child13->GenerateSignature();

        TDocNode* const child04 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode* child14 = MakeBlock(Pool, 0, child04, HT_DIV);

        child14->Props.Class = 3;
        child14->Props.Width = 5;
        child14->GenerateSignature();

        TDocNode* const child05 = MakeBlock(Pool, 0, root, HT_DIV);
        TDocNode* child15 = MakeBlock(Pool, 0, child05, HT_DIV);

        child15->Props.Class = 3;
        child15->Props.Width = 5;
        child15->GenerateSignature();

        UNIT_ASSERT(*root == *root && !(*root != *root));
        UNIT_ASSERT(*child04 == *child04 && !(*child04 != *child04));
        UNIT_ASSERT(*child14 == *child14 && !(*child14 != *child14));

        UNIT_ASSERT(*child04 != *child14 && !(*child04 == *child14));
        UNIT_ASSERT(*child14 != *child04 && !(*child14 == *child04));

        UNIT_ASSERT(*child04 != *child01 && !(*child04 == *child01));
        UNIT_ASSERT(*child04 != *child02 && !(*child04 == *child02));
        UNIT_ASSERT(*child04 != *child03 && !(*child04 == *child03));

        UNIT_ASSERT(*child04 == *child05 && !(*child04 != *child05));

        UNIT_ASSERT(*child14 != *child11 && !(*child14 == *child11));
        UNIT_ASSERT(*child14 != *child12 && !(*child14 == *child12));

        UNIT_ASSERT(*child14 != *child13 && !(*child14 == *child13));
        UNIT_ASSERT(*child13 != *child14 && !(*child13 == *child14));

        UNIT_ASSERT(*child14 == *child15 && !(*child14 != *child15));
        UNIT_ASSERT(*child15 == *child14 && !(*child15 != *child14));
    }

    void ReplaceTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        /*
         root <- child01=table <- child11=tr <- child21=td <- child31=p
         .....^- child02=ul    <- child12=li <- child22=p
         */

        TDocNode * root = MakeBlock(Pool, 0, nullptr, HT_any);
        TDocNode * child01 = MakeBlock(Pool, 0, root, HT_TABLE);
        TDocNode * child11 = MakeBlock(Pool, 0, child01, HT_TR);
        TDocNode * child21 = MakeBlock(Pool, 0, child11, HT_TD);
        TDocNode * child31 = MakeBlock(Pool, 0, child21, HT_P);

        TDocNode * child02 = MakeBlock(Pool, 0, root, HT_UL);
        TDocNode * child12 = MakeBlock(Pool, 0, child02, HT_LI);
        TDocNode * child22 = MakeBlock(Pool, 0, child12, HT_P);

        child01->Replace();
        child02->Replace();

        UNIT_ASSERT(child01->Front() == child01->Back() && child01->Back() == child31);
        UNIT_ASSERT(child01->Props.Tag == HT_DIV);

        UNIT_ASSERT(child02->Front() == child02->Back() && child02->Back() == child22);
        UNIT_ASSERT(child02->Props.Tag == HT_DIV);
    }

    void DetelescopeTest() {
        using namespace NSegm;
        using namespace NSegm::NPrivate;
        /*
         root <- child01=h1 <- child11=div <- child21=div <- child31=address <- child41=p
         ..................................^- child22=div
         ..................................^- child23=div
         */
        TDocNode * root = MakeBlock(Pool, 0, nullptr, HT_any);
        TDocNode * child01 = MakeBlock(Pool, 0, root, HT_H1);

        child01->Props.BlockMarkers.PollCSS = 1;

        TDocNode * child11 = MakeBlock(Pool, 0, child01, HT_DIV);

        child11->Props.BlockMarkers.AdsCSS = 1;

        TDocNode * child21 = MakeBlock(Pool, 0, child11, HT_DIV);
        TDocNode * child22 = MakeBlock(Pool, 0, child11, HT_DIV);
        TDocNode * child23 = MakeBlock(Pool, 0, child11, HT_DIV);
        TDocNode * child31 = MakeBlock(Pool, 0, child21, HT_ADDRESS);
        TDocNode * child41 = MakeBlock(Pool, 0, child31, HT_P);

        child01->Collapse();
        child21->Collapse();

        UNIT_ASSERT(child01->Front() == child21 && child01->Back() == child23);
        UNIT_ASSERT(child01 == child21->Parent);
        UNIT_ASSERT(child01 == child22->Parent);
        UNIT_ASSERT(child01 == child23->Parent);
        UNIT_ASSERT(3 == child01->Size());
        UNIT_ASSERT(HT_H1 == child01->Props.Tag);
        UNIT_ASSERT(child01->Props.BlockMarkers.PollCSS);
        UNIT_ASSERT(child01->Props.BlockMarkers.AdsCSS);
        UNIT_ASSERT(!child01->Props.BlockMarkers.HeaderCSS);
        UNIT_ASSERT(HT_ADDRESS == child21->Props.Tag);
        UNIT_ASSERT(child21->Front() == child41);
        UNIT_ASSERT(child41->Parent == child21);
    }

};

UNIT_TEST_SUITE_REGISTRATION(TTreeBuilderUtilsTest)
;
