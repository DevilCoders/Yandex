#include <kernel/remorph/core/input_tree.h>
#include <kernel/remorph/core/input_tree_repr.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/system/defaults.h>

#define CHECK_INPUT(input, str) \
    UNIT_ASSERT_EQUAL_C(str ""sv, InputTreeToString(input, TCharRepr()), ", actual=" << InputTreeToString(input, TCharRepr()));

using namespace NRemorph;

namespace {

struct TCharRepr {
    inline static void GetLabel(IOutputStream& output, char ch) {
        if (ch > 31) {
            output << ch;
        } else {
            output << "i" << ui16(ch);
        }
    }
};

struct TCharFilter {
    const char C;

    TCharFilter(char c)
        : C(c)
    {
    }

    inline bool operator ()(char c) const {
        return C == c;
    }
};

struct TSymbolAct {
    const char StopChar;
    TString& Res;

    TSymbolAct(TString& s, char stopChar)
        : StopChar(stopChar)
        , Res(s)
    {
    }

    TSymbolAct(TString& s)
        : TSymbolAct(s, 0)
    {
    }

    inline bool operator ()(char c) const {
        Res.append(c);
        return 0 == StopChar || StopChar != c;
    }
};

struct TBranchAct {
    TStringOutput Out;

    TBranchAct(TString& s)
        : Out(s)
    {
    }

    inline bool operator ()(const TVector<char>& branch, const TVector<size_t>& track) {
        UNIT_ASSERT_EQUAL(branch.size(), track.size());
        Out << '|';
        for (size_t i = 0; i < branch.size(); ++i) {
            Out << track[i] << branch[i];
        }
        return true;
    }
};

}

Y_UNIT_TEST_SUITE(RemorphCoreInputTree) {
    Y_UNIT_TEST(Create) {
        TInputTree<char> input;
        CHECK_INPUT(input, "[0,i0,l0,b0,t]");
    }

    Y_UNIT_TEST(Clear) {
        TInputTree<char> input;

        input.Clear();
        CHECK_INPUT(input, "[0,i0,l0,b0,t]");
    }

    Y_UNIT_TEST(Branch) {
        TInputTree<char> input;
        TStringBuf symbols = "abcd";

        // a-b-c-d
        input.Fill(symbols.begin(), symbols.end());
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t]");

        // a-b-e-f
        UNIT_ASSERT(input.CreateBranch(2, 3, 'e', true));
        UNIT_ASSERT(input.CreateBranch(3, 4, 'f', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,e,l1,b1,t][4,f,l1,b0,t]");

        // a-b-e-f
        // \-g-f
        UNIT_ASSERT(input.CreateBranch(1, 3, 'g', false));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,e,l1,b1,t][4,f,l1,b0,t]"
                                                      "[2,g,l2,b1,t][3,f,l1,b0,t]");

        // a-b-e-f
        // | \-h
        // \-g-f
        UNIT_ASSERT(input.CreateBranch(2, 4, 'h', false));
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,e,l1,b1,t][4,f,l1,b0,t]"
                                                                   "[3,h,l2,b0,t]"
                                                      "[2,g,l2,b1,t][3,f,l1,b0,t]");
    }

    Y_UNIT_TEST(BranchAdvanced) {
        TInputTree<char> input;
        TStringBuf symbols = "abcd";

        // a-b-c-d
        // | | \-g
        // | \-f-d
        // |   \-g
        // \-e-c-d
        //   | \-g
        //   \-f-d
        //     \-g
        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 2, 'e', false));
        UNIT_ASSERT(input.CreateBranch(2, 3, 'f', false));
        UNIT_ASSERT(input.CreateBranch(3, 4, 'g', false));
        CHECK_INPUT(input, "[0,i0,l0,b8,t][1,a,l1,b8,t][2,b,l1,b4,t][3,c,l1,b2,t][4,d,l1,b0,t]"
                                                                                "[4,g,l1,b0,t]"
                                                                   "[3,f,l1,b2,t][4,d,l1,b0,t]"
                                                                                "[4,g,l1,b0,t]"
                                                      "[2,e,l1,b4,t][3,c,l1,b2,t][4,d,l1,b0,t]"
                                                                                "[4,g,l1,b0,t]"
                                                                   "[3,f,l1,b2,t][4,d,l1,b0,t]"
                                                                                "[4,g,l1,b0,t]");

        // a-b-h-d
        // |   \-g
        // \-e-h-d
        //     \-g
        UNIT_ASSERT(input.CreateBranch(2, 3, 'h', true));
        CHECK_INPUT(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b2,t][3,h,l1,b2,t][4,d,l1,b0,t]"
                                                                                "[4,g,l1,b0,t]"
                                                      "[2,e,l1,b2,t][3,h,l1,b2,t][4,d,l1,b0,t]"
                                                                                "[4,g,l1,b0,t]");
    }

    Y_UNIT_TEST(Filter) {
        TInputTree<char> input;
        TStringBuf symbols = "abcd";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.Filter(TCharFilter('a')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,f][3,c,l1,b1,f][4,d,l1,b0,f]");

        UNIT_ASSERT(input.Filter(TCharFilter('c')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,f]");

        UNIT_ASSERT(input.Filter(TCharFilter('d')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t]");

        UNIT_ASSERT(!input.Filter(TCharFilter('h')));
        CHECK_INPUT(input, "[0,i0,l0,b0,f]");

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t][3,e,l2,b0,t][2,f,l2,b1,t][3,d,l1,b0,t]");

        // a-b-c-d
        // | \-e
        // \-f-d
        UNIT_ASSERT(input.Filter(TCharFilter('a')));
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,f][3,c,l1,b1,f][4,d,l1,b0,f][3,e,l2,b0,f][2,f,l2,b1,f][3,d,l1,b0,f]");

        // a-b-c-d
        //   \-e
        UNIT_ASSERT(input.Filter(TCharFilter('b')));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b2,t][3,c,l1,b1,f][4,d,l1,b0,f][3,e,l2,b0,f]");

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t][3,e,l2,b0,t][2,f,l2,b1,t][3,d,l1,b0,t]");

        // a-f-d
        UNIT_ASSERT(input.Filter(TCharFilter('f')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,f,l2,b1,t][3,d,l1,b0,f]");

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t][3,e,l2,b0,t][2,f,l2,b1,t][3,d,l1,b0,t]");

        // a-b-c-d
        // \-f-d
        UNIT_ASSERT(input.Filter(TCharFilter('d')));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t][2,f,l2,b1,t][3,d,l1,b0,t]");
    }

    Y_UNIT_TEST(AdvancedBranchFilter) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        // a-b-c-d-e
        // |-g-c-d-e
        // |-h-d-e
        // \-i-e
        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 2, 'f', false));
        UNIT_ASSERT(input.CreateBranch(1, 3, 'g', false));
        UNIT_ASSERT(input.CreateBranch(1, 4, 'h', false));
        CHECK_INPUT(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                      "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                      "[2,g,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]"
                                                      "[2,h,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TCharFilter('a')));
        CHECK_INPUT(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b1,f][3,c,l1,b1,f][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                      "[2,f,l1,b1,f][3,c,l1,b1,f][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                      "[2,g,l2,b1,f][3,d,l1,b1,f][4,e,l1,b0,f]"
                                                      "[2,h,l3,b1,f][3,e,l1,b0,f]");

        UNIT_ASSERT(input.Filter(TCharFilter('e')));
        CHECK_INPUT(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                      "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                      "[2,g,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]"
                                                      "[2,h,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TCharFilter('d')));
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,f]"
                                                      "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,f]"
                                                      "[2,g,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,f]");

        UNIT_ASSERT(input.CreateBranch(4, 5, 'i', true));
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,i,l1,b0,t]"
                                                      "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,i,l1,b0,t]"
                                                      "[2,g,l2,b1,t][3,d,l1,b1,t][4,i,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TCharFilter('g')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,g,l2,b1,t][3,d,l1,b1,f][4,i,l1,b0,f]");

        UNIT_ASSERT(input.CreateBranch(4, 5, 'j', false));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,g,l2,b2,t][3,d,l1,b2,f][4,i,l1,b0,f]"
                                                                                "[4,j,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TCharFilter('i')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,g,l2,b1,t][3,d,l1,b1,t][4,i,l1,b0,t]");

        UNIT_ASSERT(!input.Filter(TCharFilter('x')));
        CHECK_INPUT(input, "[0,i0,l0,b0,f]");
    }

    Y_UNIT_TEST(Replace) {
        TInputTree<char> input;
        TStringBuf symbols = "abcd";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 2, 'B', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(1, 3, 'E', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,E,l2,b1,t][3,d,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 3, 'E', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,E,l2,b1,t][3,d,l1,b0,t]");
    }

    Y_UNIT_TEST(ReplaceAtStart) {
        TInputTree<char> input;
        TStringBuf symbols = "abc";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(0, 1, 'A', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,A,l1,b1,t][2,b,l1,b1,t][3,c,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(0, 2, 'D', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,D,l2,b1,t][2,c,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(0, 2, 'D', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,D,l2,b1,t][2,c,l1,b0,t]");
    }

    Y_UNIT_TEST(ReplaceAtEnd) {
        TInputTree<char> input;
        TStringBuf symbols = "abc";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(2, 3, 'C', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(1, 3, 'D', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,D,l2,b0,t]");

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 3, 'D', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,D,l2,b0,t]");
    }

    Y_UNIT_TEST(ReplaceSeq) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 2, 'B', true));
        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(ReplaceMulti) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 2, 'B', false);
        input.CreateBranch(2, 4, 'F', false);
        input.CreateBranch(1, 4, 'G', false);
        CHECK_INPUT(input, "[0,i0,l0,b5,t][1,a,l1,b5,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,B,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,G,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(0, 1, 'A', true));
        CHECK_INPUT(input, "[0,i0,l0,b5,t][1,A,l1,b5,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,B,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,G,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(4, 5, 'E', true));
        CHECK_INPUT(input, "[0,i0,l0,b5,t][1,A,l1,b5,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,E,l1,b0,t]"
                                                                   "[3,F,l2,b1,t][4,E,l1,b0,t]"
                                                      "[2,B,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,E,l1,b0,t]"
                                                                   "[3,F,l2,b1,t][4,E,l1,b0,t]"
                                                      "[2,G,l3,b1,t][3,E,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(1, 4, 'H', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,A,l1,b1,t][2,H,l3,b1,t][3,E,l1,b0,t]");
    }

    Y_UNIT_TEST(Overlap) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', false));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,C,l2,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', false));
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,C,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', false));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                      "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', false));
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,C,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(OverlapSpoilingReplace) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]");

        // No continuatiion for B-branch
        UNIT_ASSERT(!input.CreateBranch(1, 3, 'B', false));
        UNIT_ASSERT(!input.CreateBranch(1, 3, 'B', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        // No continuatiion for C-branch
        UNIT_ASSERT(!input.CreateBranch(2, 4, 'C', false));
        UNIT_ASSERT(!input.CreateBranch(2, 4, 'C', true));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(OverlapCautiousReplace) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', false));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,C,l2,b1,t][4,e,l1,b0,t]");

        // B-branch replaces 'bc', not touching C-branch
        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', true));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', false));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                      "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        // C-branch replaces 'cd', not touching B-branch
        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(OverlapPreciselyCautiousReplace) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        UNIT_ASSERT(input.CreateBranch(0, 2, 'A', false));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]"
                                         "[1,A,l2,b1,t][2,C,l2,b1,t][3,e,l1,b0,t]");

        // F-branch replaces 'b', not touching A-branch, C-branch (as it follows A-branch), and also keeps 'cd' removed
        UNIT_ASSERT(input.CreateBranch(1, 4, 'F', true));
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b1,t][2,F,l3,b1,t][3,e,l1,b0,t]"
                                         "[1,A,l2,b1,t][2,C,l2,b1,t][3,e,l1,b0,t]");
    }

    Y_UNIT_TEST(RemovingFilter) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 2, 'B', false);
        input.CreateBranch(2, 3, 'C', false);
        input.CreateBranch(3, 4, 'D', false);
        CHECK_INPUT(input, "[0,i0,l0,b8,t][1,a,l1,b8,t][2,b,l1,b4,t][3,c,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                                "[4,D,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,C,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                                "[4,D,l1,b1,t][5,e,l1,b0,t]"
                                                      "[2,B,l1,b4,t][3,c,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                                "[4,D,l1,b1,t][5,e,l1,b0,t]"
                                                                   "[3,C,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                                "[4,D,l1,b1,t][5,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TCharFilter('C')));
        CHECK_INPUT(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b2,t][3,C,l1,b2,t][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                                                "[4,D,l1,b1,f][5,e,l1,b0,f]"
                                                      "[2,B,l1,b2,t][3,C,l1,b2,t][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                                                "[4,D,l1,b1,f][5,e,l1,b0,f]");
    }

    Y_UNIT_TEST(OverlapRemovingFilter) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(2, 4, 'F', false);
        input.CreateBranch(1, 3, 'G', true);
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,F,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,G,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TCharFilter('b')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,F,l2,b1,f][4,e,l1,b0,f]");
    }

    Y_UNIT_TEST(OverlapPreciselyRemovingFilter) {
        TInputTree<char> input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(2, 4, 'F', false);
        input.CreateBranch(1, 3, 'G', true);
        CHECK_INPUT(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,F,l2,b1,t][4,e,l1,b0,t]"
                                                      "[2,G,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TCharFilter('b')));
        CHECK_INPUT(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,F,l2,b1,f][4,e,l1,b0,f]");

        UNIT_ASSERT(!input.CreateBranch(1, 3, 'H', false));
        UNIT_ASSERT(!input.CreateBranch(1, 3, 'H', true));
        UNIT_ASSERT(!input.CreateBranch(3, 4, 'H', false));
        UNIT_ASSERT(!input.CreateBranch(3, 4, 'H', true));
    }

    Y_UNIT_TEST(TraverseSymbols) {
        TInputTree<char> input;
        TStringBuf symbols = "abcd";

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                                   "[3,e,l2,b0,t]"
                                                      "[2,f,l2,b1,t][3,d,l1,b0,t]");

        TString res1;
        TSymbolAct act1(res1);
        input.TraverseSymbols(act1);
        UNIT_ASSERT_STRINGS_EQUAL(res1, TStringBuf("afdbec"));

        TString res2;
        TSymbolAct act2(res2, 'e');
        input.TraverseSymbols(act2);
        UNIT_ASSERT_STRINGS_EQUAL(res2, TStringBuf("afdbe"));
    }

    Y_UNIT_TEST(TraverseBranches) {
        TInputTree<char> input;
        TStringBuf symbols = "abcd";

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        CHECK_INPUT(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                                   "[3,e,l2,b0,t]"
                                                      "[2,f,l2,b1,t][3,d,l1,b0,t]");

        TString res;
        TBranchAct act(res);
        input.TraverseBranches(act);
        UNIT_ASSERT_STRINGS_EQUAL(res, TStringBuf("|0a0b0c0d|0a0b1e|0a1f0d"));
    }
}
