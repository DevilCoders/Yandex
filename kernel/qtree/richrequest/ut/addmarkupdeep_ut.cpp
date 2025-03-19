#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/testing/unittest/registar.h>


/*    ________A________
     /        |        \
    /         |         \
   /          |          \
  B           C           D
  |          / \         /|\
  |         /   \       / | \
  E        F     G     H  I  J
*/


namespace {
    class TModelTree {
        TRichNodePtr Tree;
public:
        TModelTree() {
            Tree = CreateEmptyRichNode();      // A

            A()->Children.Append(CreateEmptyRichNode());   // B
            A()->Children.Append(CreateEmptyRichNode());   // C
            A()->Children.Append(CreateEmptyRichNode());   // D

            B()->Children.Append(CreateEmptyRichNode());   // E

            C()->Children.Append(CreateEmptyRichNode());   // F
            C()->Children.Append(CreateEmptyRichNode());   // G

            D()->Children.Append(CreateEmptyRichNode());   // H
            D()->Children.Append(CreateEmptyRichNode());   // I
            D()->Children.Append(CreateEmptyRichNode());   // J
        }

        TRichRequestNode* A() {
            return Tree.Get();
        }

        const TRichRequestNode* A() const {
            return Tree.Get();
        }

#define GETTER(name, parent, i)                     \
        TRichRequestNode* name() {                  \
            return parent()->Children[i].Get();     \
        }                                           \
        const TRichRequestNode* name() const {      \
            return parent()->Children[i].Get();     \
        }

        GETTER(B, A, 0)
        GETTER(C, A, 1)
        GETTER(D, A, 2)

        GETTER(E, B, 0)

        GETTER(F, C, 0)
        GETTER(G, C, 1)

        GETTER(H, D, 0)
        GETTER(I, D, 1)
        GETTER(J, D, 2)
#undef GETTER
    };
}

static TString ToString(const NSearchQuery::TRange& r) {
    return Sprintf("(%d %d)", r.Beg, r.End);
}

class TAddMarkupDeepTest : public TTestBase {
    UNIT_TEST_SUITE(TAddMarkupDeepTest);
        UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();


static NSearchQuery::TMarkupDataPtr CreateSyn(const char *s) {
    return new TSynonym(CreateRichNode(UTF8ToWide(s), TCreateTreeOptions()), TE_MARK);
}

#define SYNS(root) root->Markup().GetItems<TSynonym>()
#define LAST_SYN_TREE(root) SYNS(tree.root()).back().GetDataAs<TSynonym>().SubTree

#define ASSERT_COUNT(root) UNIT_ASSERT_EQUAL(SYNS(tree.root()).size(), ++cnt##root)

#define ASSERT_RANGE(root, a, b, str) \
    UNIT_ASSERT_EQUAL_C(SYNS(root).back().Range, TRange(a, b), ToString(SYNS(root).back().Range) + str);


#define ASSERT_ADD_SYN_(root, a, b, str)                                        \
    UNIT_ASSERT( root->AddMarkupDeep(*a, *b, CreateSyn(str), true));            \
    UNIT_ASSERT(!root->AddMarkupDeep(*a, *b, CreateSyn(str " false"), false));

#define ASSERT_ADD_SYN(rootAdd, rootCmp, a, b, rangeBeg, rangeEnd, str) \
    ASSERT_ADD_SYN_(tree.rootAdd(), a, b, str);                         \
    ASSERT_COUNT(rootCmp);                                              \
    ASSERT_RANGE(tree.rootCmp(), rangeBeg, rangeEnd, str);

#define ASSERT_ADD_SYN_NOT(root, a, b, str)                                             \
    UNIT_ASSERT(!tree.root()->AddMarkupDeep(*a, *b, CreateSyn(str), true));             \
    UNIT_ASSERT(!tree.root()->AddMarkupDeep(*a, *b, CreateSyn(str " false"), false));


#define ASSERT_ADD_(rootAdd, rootCmp, a, b, allowSyn, rangeBeg, rangeEnd)                                                       \
{                                                                                                                               \
    using NSearchQuery::TRange;                                                                                                 \
    using NSearchQuery::TMarkup;                                                                                                \
    UNIT_ASSERT(tree.rootAdd()->AddMarkupDeep(*tree.a(), *tree.b(), CreateSyn(#rootAdd " " #rootCmp " " #a " " #b " " #allowSyn), allowSyn));   \
    ASSERT_COUNT(rootCmp);                                                                                                      \
    ASSERT_RANGE(tree.rootCmp(), rangeBeg, rangeEnd, #rootAdd " " #rootCmp " " #a " " #b " " #allowSyn)                         \
}

#define ASSERT_ADD2(rootAdd, rootCmp, a, b, rangeBeg, rangeEnd)     \
{                                                                   \
    ASSERT_ADD_(rootAdd, rootCmp, a, b, true, rangeBeg, rangeEnd);  \
    ASSERT_ADD_(rootAdd, rootCmp, a, b, false, rangeBeg, rangeEnd)  \
}

#define ASSERT_ADD(root, a, b, rangeBeg, rangeEnd) \
    ASSERT_ADD2(root, root, a, b, rangeBeg, rangeEnd)

#define ASSERT_ADD_NOT(root, a, b) \
    UNIT_ASSERT(!tree.root()->AddMarkupDeep(*tree.a(), *tree.b(), CreateSyn(#root " " #a " " #b), false));

    void Test() {
        TModelTree tree;

        size_t cntA = 0;

        ASSERT_ADD(A, A, A, 0, 2);
        ASSERT_ADD(A, A, J, 0, 2);
        ASSERT_ADD(A, E, A, 0, 2);
        ASSERT_ADD(A, B, D, 0, 2);
        ASSERT_ADD(A, E, J, 0, 2);
        TRichNodePtr synA_BD = LAST_SYN_TREE(A);

        ASSERT_ADD(A, B, C, 0, 1);
        ASSERT_ADD(A, E, C, 0, 1);
        ASSERT_ADD(A, B, G, 0, 1);
        ASSERT_ADD(A, E, G, 0, 1);
        TRichNodePtr synA_BC = LAST_SYN_TREE(A);

        ASSERT_ADD(A, C, D, 1, 2);
        ASSERT_ADD(A, C, J, 1, 2);
        ASSERT_ADD(A, F, D, 1, 2);
        ASSERT_ADD(A, F, J, 1, 2);
        TRichNodePtr synA_CD = LAST_SYN_TREE(A);

        size_t cntB = 0;
        ASSERT_ADD(B, B, B, 0, 0);
        TRichNodePtr synB_EE = LAST_SYN_TREE(B);

        size_t cntC = 0;
        ASSERT_ADD(C, F, G, 0, 1);
        ASSERT_ADD2(A, C, F, G, 0, 1);
        TRichNodePtr synC_FG = LAST_SYN_TREE(C);

        ASSERT_ADD(A, D, D, 2, 2);
        TRichNodePtr synA_DD = LAST_SYN_TREE(A);

        size_t cntD = 0;
        ASSERT_ADD(D, I, J, 1, 2);
        ASSERT_ADD2(A, D, H, J, 0, 2);
        TRichNodePtr synD_HJ = LAST_SYN_TREE(D);
        ASSERT_ADD2(A, D, I, J, 1, 2);
        TRichNodePtr synD_IJ = LAST_SYN_TREE(D);
        ASSERT_ADD2(A, D, H, I, 0, 1);
        TRichNodePtr synD_HI = LAST_SYN_TREE(D);

        ASSERT_ADD_NOT(A, D, C); // C < D

        ASSERT_ADD_NOT(A, G, H);

        ASSERT_ADD_NOT(C, I, J);

        ASSERT_ADD_NOT(A, I, D);
        ASSERT_ADD_NOT(A, D, I);

        using NSearchQuery::TRange;
        using NSearchQuery::TMarkup;
        // in one synonym
        ASSERT_ADD_SYN_(tree.A(), synA_BD, synA_BD, "synA_BD");
        UNIT_ASSERT_EQUAL(SYNS(synA_BD).size(), 1);
        ASSERT_RANGE(synA_BD, 0, synA_BD->Children.size() - 1, "synA_BD");

        ASSERT_ADD_SYN_(tree.A(), synA_BD->Children[0], synA_BD->Children.back(), "synA_BD[0-back]");
        UNIT_ASSERT_EQUAL(SYNS(synA_BD).size(), 2);
        ASSERT_RANGE(synA_BD, 0, synA_BD->Children.size() - 1, "synA_BD[0-back]");

        ASSERT_ADD_SYN_(tree.A(), synC_FG->Children[0], synC_FG->Children[2], "synC_FG[0-2]");
        UNIT_ASSERT_EQUAL(SYNS(synC_FG).size(), 1);
        ASSERT_RANGE(synC_FG, 0, 2, "synC_FG[0-2]");

        // from synonym to same node, if the only child
        ASSERT_ADD_SYN(A, B, synB_EE, tree.E(), 0, 0, "synB_EE - E");
        ASSERT_ADD_SYN(A, B, tree.E(), synB_EE, 0, 0, "E - synB_EE");

        ASSERT_ADD_SYN(A, B, synB_EE->Children[0], tree.E(), 0, 0, "synB_EE[0] - E");
        ASSERT_ADD_SYN(A, B, tree.E(), synB_EE->Children.back(), 0, 0, "E - synB_EE[back]");

        ASSERT_ADD_SYN_NOT(A, synB_EE->Children.back(), tree.E(), "synB_EE[back] - E");

        // from synonym to same node, if not the only child
        ASSERT_ADD_SYN(A, A, synD_HJ, tree.D(), 2, 2, "synD_HJ - D");
        ASSERT_ADD_SYN(A, A, tree.D(), synD_HJ, 2, 2, "D - synD_HJ");

        ASSERT_ADD_SYN(A, A, tree.D(), synD_HJ->Children.back(), 2, 2, "D - synD_HJ[back]");
        ASSERT_ADD_SYN(A, A, synD_HJ->Children[0], tree.D(), 2, 2, "synD_HJ[0] - D");

        ASSERT_ADD_SYN_NOT(A, synD_IJ, tree.D(), "synD_IJ - D");

        ASSERT_ADD_SYN(A, A, tree.D(), synD_IJ, 2, 2, "D - synD_IJ");

        ASSERT_ADD_SYN(A, A, tree.D(), synD_IJ->Children.back(), 2, 2, "D - synD_IJ[back]");

        // from synonym to child of the same node
        ASSERT_ADD_SYN(A, D, tree.H(), synD_IJ, 0, 2, "H - synD_IJ");
        ASSERT_ADD_SYN(A, D, tree.H(), synD_IJ->Children.back(), 0, 2, "H - synD_IJ[back]");

        ASSERT_ADD_SYN(A, D, tree.I(), synD_IJ, 1, 2, "I - synD_IJ");
        ASSERT_ADD_SYN(A, D, tree.I(), synD_IJ->Children.back(), 1, 2, "I - synD_IJ[back]");

        ASSERT_ADD_SYN(A, D, synD_HI, tree.I(), 0, 1, "synD_HI - I");
        ASSERT_ADD_SYN(A, D, synD_HI->Children[0], tree.I(), 0, 1, "synD_HI[0] - I");

        ASSERT_ADD_SYN(A, D, synD_HI, tree.J(), 0, 2, "synD_HI - J");
        ASSERT_ADD_SYN(A, D, synD_IJ, tree.J(), 1, 2, "synD_IJ - J");

        ASSERT_ADD_SYN_NOT(A, tree.I(), synD_HJ->Children.back(), "I - synD_HJ[back]");

        ASSERT_ADD_SYN_NOT(A, synD_HJ->Children[0], tree.I(), "synD_HJ[0] - I");
        ASSERT_ADD_SYN_NOT(A, synD_HJ, tree.I(), "synD_HJ - I");

        // from synonym to child of the other node
        // or from synonym to synonym of the other node
        ASSERT_ADD_SYN(A, A, tree.B(), synD_HJ->Children.back(), 0, 2, "B - synD_HJ[back]");

        ASSERT_ADD_SYN(A, A, synB_EE, tree.D(), 0, 2, "synB_EE - D");

        ASSERT_ADD_SYN(A, A, synB_EE->Children[0], synD_HJ, 0, 2, "synB_EE[0] - synD_HJ");

        ASSERT_ADD_SYN(A, A, synC_FG->Children[0], synD_HJ, 1, 2, "synC_FG - synD_HJ");

        ASSERT_ADD_SYN_NOT(A, synC_FG->Children[1], synD_HJ, "synC_FG[1] - synD_HJ");
        ASSERT_ADD_SYN_NOT(A, synC_FG->Children[1], synD_HJ->Children[1], "synC_FG[1] - synD_HJ[1]");
        ASSERT_ADD_SYN_NOT(A, synC_FG, synD_HJ->Children[0], "synC_FG - synD_HJ[0]");
        ASSERT_ADD_SYN_NOT(A, synC_FG, synD_HJ->Children[1], "synC_FG - synD_HJ[1]");

        ASSERT_ADD_SYN_NOT(A, tree.C(), synD_HJ->Children[0], "C - synD_HJ[0]");
        ASSERT_ADD_SYN_NOT(A, tree.C(), synD_HJ->Children[1], "C - synD_HJ[1]");

        ASSERT_ADD_SYN_NOT(A, tree.G(), synD_HJ, "G - synD_HJ");

        // through the root
        ASSERT_ADD_SYN(A, A, tree.B(), synA_CD->Children.back(), 0, 2, "B - synA_CD[back]");
        ASSERT_ADD_SYN(A, A, synA_BC, tree.D(), 0, 2, "synA_BC - D");
        ASSERT_ADD_SYN(A, A, synA_BC, tree.A(), 0, 2, "synA_BC - A");
        ASSERT_ADD_SYN(A, A, tree.A(), synA_CD, 0, 2, "A - synA_CD");
        ASSERT_ADD_SYN(A, A, tree.B(), synA_BC->Children.back(), 0, 1, "B - synA_BC[back]");
        ASSERT_ADD_SYN(A, A, synA_BC->Children[0], tree.C(), 0, 1, "synA_BC[0] - C");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TAddMarkupDeepTest);

#undef SYNS
#undef LAST_SYN_TREE

#undef ASSERT_COUNT
#undef ASSERT_RANGE

#undef ASSERT_ADD_SYN_
#undef ASSERT_ADD_SYN
#undef ASSERT_ADD_SYN_NOT

#undef ASSERT_ADD_
#undef ASSERT_ADD2
#undef ASSERT_ADD
#undef ASSERT_ADD_NOT
