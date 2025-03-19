#include <kernel/remorph/core/poli_input_compat.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/bitmap.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

#define PI_CHECK(input, model) \
    UNIT_ASSERT_STRINGS_EQUAL(::ToString(input), model)

using namespace NRemorph;

namespace {

using TTestInput = TPoliInputCompat<char>;

struct TTestNode {
    char Symbol;
    size_t SymbolLength;
    size_t Branches;
    bool Accepted;
    TVector<TTestNode> Children;

    inline void Dump(IOutputStream& output) const {
        struct TElem {
            size_t Depth;
            const TTestNode* Node;
        };

        TVector<TElem> stack;
        stack.push_back(TElem{0, this});

        while (!stack.empty()) {
            TElem elem = stack.back();
            Y_ASSERT(elem.Node);
            stack.pop_back();
            output << '[' << elem.Depth << ',';
            if (elem.Node->Symbol > 31) {
                output << elem.Node->Symbol;
            } else {
                output << 'i' << ui16(elem.Node->Symbol);
            }
            output << ",l" << elem.Node->SymbolLength;
            output << ",b" << elem.Node->Branches;
            output << ',' << (elem.Node->Accepted ? 't' : 'f');
            output << ']';
            for (const auto& child: elem.Node->Children) {
                stack.push_back(TElem{elem.Depth + 1, &child});
            }
        }
    }
};

inline void BuildTestTree(TTestNode& rootNode, const TTestInput& input) {
    struct TElem {
        TTestNode* Node;
        TTestNode* ParentNode;
        size_t Pos;
        bool Parsed;
    };

    const TTestInput::TRawInput& rawInput = input.GetRawInput();
    const TTestInput::TResolver& resolver = input.GetResolver();
    if (!rawInput.GetLength()) {
        return;
    }
    TVector<TElem> stack(1, TElem{&rootNode, nullptr, 0, false});
    TVector<TVector<size_t>> resolveCache(rawInput.GetLength());
    while (!stack.empty()) {
        if (!stack.back().Parsed) {
            stack.back().Parsed = true;
            TElem elem = stack.back();
            if (elem.Pos < rawInput.GetLength()) {
                if (!resolveCache[elem.Pos]) {
                    TDynBitMap alts;
                    alts.Set(0, rawInput.GetAltsCount(elem.Pos));
                    resolver(alts, elem.Pos);
                    for (size_t alt = alts.FirstNonZeroBit(); alt != alts.Size(); alt = alts.NextNonZeroBit(alt)) {
                        resolveCache[elem.Pos].push_back(alt);
                    }
                }
                const TVector<size_t>& alts = resolveCache[elem.Pos];
                elem.Node->Children.reserve(alts.size());
                for (TVector<size_t>::const_reverse_iterator alt = alts.rbegin(); alt != alts.rend(); ++alt) {
                    size_t step = rawInput.GetSymbolLength(elem.Pos, *alt);
                    elem.Node->Children.push_back(TTestNode{rawInput.GetSymbol(elem.Pos, *alt), step, 0,
                                                  input.GetAccept(elem.Pos, *alt), {}});
                    stack.push_back(TElem{&elem.Node->Children.back(), elem.Node, elem.Pos + step, false});
                }
            }
        } else {
            const TElem& elem = stack.back();
            if (elem.ParentNode) {
                elem.ParentNode->Branches += elem.Node->Branches ? elem.Node->Branches : 1;
            }
            stack.pop_back();
        }
    }
}

struct TTestAcceptor {
    char Symbol;

    TTestAcceptor(char symbol)
        : Symbol(symbol)
    {
    }

    inline bool operator ()(char symbol) const {
        return symbol == Symbol;
    }
};

struct TTestSymbolAction {
    IOutputStream& Output;
    char StopSymbol;

    TTestSymbolAction(IOutputStream& output, char stopSymbol = 0)
        : Output(output)
        , StopSymbol(stopSymbol)
    {
    }

    inline bool operator ()(char symbol) const {
        Output << symbol;
        return !StopSymbol || (symbol != StopSymbol);
    }
};

struct TTestBranchAction {
    IOutputStream& Output;

    TTestBranchAction(IOutputStream& output)
        : Output(output)
    {
    }

    inline bool operator ()(const TVector<char>& branch, const TVector<size_t>& track) const {
        UNIT_ASSERT_EQUAL(branch.size(), track.size());
        Output << '|';
        for (size_t i = 0; i < branch.size(); ++i) {
            Output << track[i] << branch[i];
        }
        return true;
    }
};

}

Y_DECLARE_OUT_SPEC(inline, TTestInput, output, input) {
    TTestNode tree{0, 0, 0, input.GetAcceptAny(), {}};
    BuildTestTree(tree, input);
    tree.Dump(output);
}

Y_UNIT_TEST_SUITE(RemorphCorePoliInputCompat) {
    Y_UNIT_TEST(Create) {
        TTestInput input;
        PI_CHECK(input, "[0,i0,l0,b0,t]");
    }

    Y_UNIT_TEST(Clear) {
        TTestInput input;
        input.Clear();
        PI_CHECK(input, "[0,i0,l0,b0,t]");
    }

    Y_UNIT_TEST(Branch) {
        TTestInput input;
        TStringBuf symbols = "abcd";

        // a-b-c-d
        input.Fill(symbols.begin(), symbols.end());
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t]");

        // a-b-e-f
        UNIT_ASSERT(input.CreateBranch(2, 3, 'e', true));
        UNIT_ASSERT(input.CreateBranch(3, 4, 'f', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,e,l1,b1,t][4,f,l1,b0,t]");

        // a-b-e-f
        // \-g-f
        UNIT_ASSERT(input.CreateBranch(1, 3, 'g', false));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,e,l1,b1,t][4,f,l1,b0,t]"
                                                   "[2,g,l2,b1,t][3,f,l1,b0,t]");

        // a-b-e-f
        // | \-h
        // \-g-f
        UNIT_ASSERT(input.CreateBranch(2, 4, 'h', false));
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,e,l1,b1,t][4,f,l1,b0,t]"
                                                                "[3,h,l2,b0,t]"
                                                   "[2,g,l2,b1,t][3,f,l1,b0,t]");
    }

    Y_UNIT_TEST(BranchAdvanced) {
        TTestInput input;
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
        PI_CHECK(input, "[0,i0,l0,b8,t][1,a,l1,b8,t][2,b,l1,b4,t][3,c,l1,b2,t][4,d,l1,b0,t]"
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
        PI_CHECK(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b2,t][3,h,l1,b2,t][4,d,l1,b0,t]"
                                                                             "[4,g,l1,b0,t]"
                                                   "[2,e,l1,b2,t][3,h,l1,b2,t][4,d,l1,b0,t]"
                                                                             "[4,g,l1,b0,t]");
    }

    Y_UNIT_TEST(Filter) {
        TTestInput input;
        TStringBuf symbols = "abcd";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.Filter(TTestAcceptor('a')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,f][3,c,l1,b1,f][4,d,l1,b0,f]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('c')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,f]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('d')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t]");

        UNIT_ASSERT(!input.Filter(TTestAcceptor('h')));
        PI_CHECK(input, "[0,i0,l0,b0,f]");

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                                "[3,e,l2,b0,t]"
                                                   "[2,f,l2,b1,t][3,d,l1,b0,t]");

        // a-b-c-d
        // | \-e
        // \-f-d
        UNIT_ASSERT(input.Filter(TTestAcceptor('a')));
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,f][3,c,l1,b1,f][4,d,l1,b0,f]"
                                                                "[3,e,l2,b0,f]"
                                                   "[2,f,l2,b1,f][3,d,l1,b0,f]");

        // a-b-c-d
        //   \-e
        UNIT_ASSERT(input.Filter(TTestAcceptor('b')));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b2,t][3,c,l1,b1,f][4,d,l1,b0,f]"
                                                                "[3,e,l2,b0,f]");

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                                "[3,e,l2,b0,t]"
                                                   "[2,f,l2,b1,t][3,d,l1,b0,t]");

        // a-f-d
        UNIT_ASSERT(input.Filter(TTestAcceptor('f')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,f,l2,b1,t][3,d,l1,b0,f]");

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                                "[3,e,l2,b0,t]"
                                                   "[2,f,l2,b1,t][3,d,l1,b0,t]");

        // a-b-c-d
        // \-f-d
        UNIT_ASSERT(input.Filter(TTestAcceptor('d')));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                   "[2,f,l2,b1,t][3,d,l1,b0,t]");
    }

    Y_UNIT_TEST(AdvancedFilterBranch) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        // a-b-c-d-e
        // |-g-c-d-e
        // |-h-d-e
        // \-i-e
        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 2, 'f', false));
        UNIT_ASSERT(input.CreateBranch(1, 3, 'g', false));
        UNIT_ASSERT(input.CreateBranch(1, 4, 'h', false));
        PI_CHECK(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                   "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                   "[2,g,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]"
                                                   "[2,h,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('a')));
        PI_CHECK(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b1,f][3,c,l1,b1,f][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                   "[2,f,l1,b1,f][3,c,l1,b1,f][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                   "[2,g,l2,b1,f][3,d,l1,b1,f][4,e,l1,b0,f]"
                                                   "[2,h,l3,b1,f][3,e,l1,b0,f]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('e')));
        PI_CHECK(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                   "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                   "[2,g,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]"
                                                   "[2,h,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('d')));
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,f]"
                                                   "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,f]"
                                                   "[2,g,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,f]");

        UNIT_ASSERT(input.CreateBranch(4, 5, 'i', true));
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,i,l1,b0,t]"
                                                   "[2,f,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,i,l1,b0,t]"
                                                   "[2,g,l2,b1,t][3,d,l1,b1,t][4,i,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('g')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,g,l2,b1,t][3,d,l1,b1,f][4,i,l1,b0,f]");

        UNIT_ASSERT(input.CreateBranch(4, 5, 'j', false));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,g,l2,b2,t][3,d,l1,b2,f][4,i,l1,b0,f]"
                                                                             "[4,j,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('i')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,g,l2,b1,t][3,d,l1,b1,t][4,i,l1,b0,t]");

        UNIT_ASSERT(!input.Filter(TTestAcceptor('x')));
        PI_CHECK(input, "[0,i0,l0,b0,f]");
    }

    Y_UNIT_TEST(Replace) {
        TTestInput input;
        TStringBuf symbols = "abcd";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 2, 'B', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l1,b1,t][3,c,l1,b1,t][4,d,l1,b0,t]");
        UNIT_ASSERT(input.CreateBranch(1, 3, 'E', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,E,l2,b1,t][3,d,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 3, 'E', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,E,l2,b1,t][3,d,l1,b0,t]");
    }

    Y_UNIT_TEST(ReplaceAtStart) {
        TTestInput input;
        TStringBuf symbols = "abc";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(0, 1, 'A', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,A,l1,b1,t][2,b,l1,b1,t][3,c,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(0, 2, 'D', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,D,l2,b1,t][2,c,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(0, 2, 'D', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,D,l2,b1,t][2,c,l1,b0,t]");
    }

    Y_UNIT_TEST(ReplaceAtEnd) {
        TTestInput input;
        TStringBuf symbols = "abc";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(2, 3, 'C', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(1, 3, 'D', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,D,l2,b0,t]");

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 3, 'D', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,D,l2,b0,t]");
    }

    Y_UNIT_TEST(ReplaceSeq) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(1, 2, 'B', true));
        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(ReplaceMulti) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 2, 'B', false);
        input.CreateBranch(2, 4, 'F', false);
        input.CreateBranch(1, 4, 'G', false);
        PI_CHECK(input, "[0,i0,l0,b5,t][1,a,l1,b5,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,B,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,G,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(0, 1, 'A', true));
        PI_CHECK(input, "[0,i0,l0,b5,t][1,A,l1,b5,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,B,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,F,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,G,l3,b1,t][3,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(4, 5, 'E', true));
        PI_CHECK(input, "[0,i0,l0,b5,t][1,A,l1,b5,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,E,l1,b0,t]"
                                                                "[3,F,l2,b1,t][4,E,l1,b0,t]"
                                                   "[2,B,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,E,l1,b0,t]"
                                                                "[3,F,l2,b1,t][4,E,l1,b0,t]"
                                                   "[2,G,l3,b1,t][3,E,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(1, 4, 'H', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,A,l1,b1,t][2,H,l3,b1,t][3,E,l1,b0,t]");
    }

    Y_UNIT_TEST(Overlap) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', false));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,C,l2,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', false));
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,C,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', false));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                   "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', false));
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,C,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(OverlapSpoilingReplace) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]");

        // No continuatiion for B-branch
        UNIT_ASSERT(!input.CreateBranch(1, 3, 'B', false));
        UNIT_ASSERT(!input.CreateBranch(1, 3, 'B', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        // No continuatiion for C-branch
        UNIT_ASSERT(!input.CreateBranch(2, 4, 'C', false));
        UNIT_ASSERT(!input.CreateBranch(2, 4, 'C', true));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(OverlapCautiousReplace) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', false));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,C,l2,b1,t][4,e,l1,b0,t]");

        // B-branch replaces 'bc', not touching C-branch
        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', true));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        input.Fill(symbols.begin(), symbols.end());

        UNIT_ASSERT(input.CreateBranch(1, 3, 'B', false));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,c,l1,b1,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                   "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        // C-branch replaces 'cd', not touching B-branch
        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,B,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");
    }

    Y_UNIT_TEST(OverlapPreciselyCautiousReplace) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        UNIT_ASSERT(input.CreateBranch(2, 4, 'C', true));
        UNIT_ASSERT(input.CreateBranch(0, 2, 'A', false));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b1,t][2,b,l1,b1,t][3,C,l2,b1,t][4,e,l1,b0,t]"
                                      "[1,A,l2,b1,t][2,C,l2,b1,t][3,e,l1,b0,t]");

        // F-branch replaces 'b', not touching A-branch, C-branch (as it follows A-branch), and also keeps 'cd' removed
        UNIT_ASSERT(input.CreateBranch(1, 4, 'F', true));
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b1,t][2,F,l3,b1,t][3,e,l1,b0,t]"
                                      "[1,A,l2,b1,t][2,C,l2,b1,t][3,e,l1,b0,t]");
    }

    Y_UNIT_TEST(RemovingFilter) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 2, 'B', false);
        input.CreateBranch(2, 3, 'C', false);
        input.CreateBranch(3, 4, 'D', false);
        PI_CHECK(input, "[0,i0,l0,b8,t][1,a,l1,b8,t][2,b,l1,b4,t][3,c,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                             "[4,D,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,C,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                             "[4,D,l1,b1,t][5,e,l1,b0,t]"
                                                   "[2,B,l1,b4,t][3,c,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                             "[4,D,l1,b1,t][5,e,l1,b0,t]"
                                                                "[3,C,l1,b2,t][4,d,l1,b1,t][5,e,l1,b0,t]"
                                                                             "[4,D,l1,b1,t][5,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('C')));
        PI_CHECK(input, "[0,i0,l0,b4,t][1,a,l1,b4,t][2,b,l1,b2,t][3,C,l1,b2,t][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                                             "[4,D,l1,b1,f][5,e,l1,b0,f]"
                                                   "[2,B,l1,b2,t][3,C,l1,b2,t][4,d,l1,b1,f][5,e,l1,b0,f]"
                                                                             "[4,D,l1,b1,f][5,e,l1,b0,f]");
    }

    Y_UNIT_TEST(OverlapRemovingFilter) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(2, 4, 'F', false);
        input.CreateBranch(1, 3, 'G', true);
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,F,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,G,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('b')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,F,l2,b1,f][4,e,l1,b0,f]");
    }

    Y_UNIT_TEST(OverlapPreciselyRemovingFilter) {
        TTestInput input;
        TStringBuf symbols = "abcde";

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(2, 4, 'F', false);
        input.CreateBranch(1, 3, 'G', true);
        PI_CHECK(input, "[0,i0,l0,b2,t][1,a,l1,b2,t][2,b,l1,b1,t][3,F,l2,b1,t][4,e,l1,b0,t]"
                                                   "[2,G,l2,b1,t][3,d,l1,b1,t][4,e,l1,b0,t]");

        UNIT_ASSERT(input.Filter(TTestAcceptor('b')));
        PI_CHECK(input, "[0,i0,l0,b1,t][1,a,l1,b1,t][2,b,l1,b1,t][3,F,l2,b1,f][4,e,l1,b0,f]");

        UNIT_ASSERT(!input.CreateBranch(1, 3, 'H', false));
        UNIT_ASSERT(!input.CreateBranch(1, 3, 'H', true));
        UNIT_ASSERT(!input.CreateBranch(3, 4, 'H', false));
        UNIT_ASSERT(!input.CreateBranch(3, 4, 'H', true));
    }

        Y_UNIT_TEST(TraverseSymbols) {
        TTestInput input;
        TStringBuf symbols = "abcd";

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                                "[3,e,l2,b0,t]"
                                                   "[2,f,l2,b1,t][3,d,l1,b0,t]");

        TStringStream result1;
        TTestSymbolAction action1(result1);
        input.TraverseSymbols(action1);
        UNIT_ASSERT_STRINGS_EQUAL(result1.Str(), TStringBuf("afdbec"));

        TStringStream result2;
        TTestSymbolAction action2(result2, 'e');
        input.TraverseSymbols(action2);
        UNIT_ASSERT_STRINGS_EQUAL(result2.Str(), TStringBuf("afdbe"));
    }

    Y_UNIT_TEST(TraverseBranches) {
        TTestInput input;
        TStringBuf symbols = "abcd";

        // a-b-c-d
        // | \-e
        // \-f-d
        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, 'f', false);
        input.CreateBranch(2, 4, 'e', false);
        PI_CHECK(input, "[0,i0,l0,b3,t][1,a,l1,b3,t][2,b,l1,b2,t][3,c,l1,b1,t][4,d,l1,b0,t]"
                                                                "[3,e,l2,b0,t]"
                                                   "[2,f,l2,b1,t][3,d,l1,b0,t]");

        TStringStream result;
        TTestBranchAction action(result);
        input.TraverseBranches(action);
        UNIT_ASSERT_STRINGS_EQUAL(result.Str(), TStringBuf("|0a0b0c0d|0a0b1e|0a1f0d"));
    }
}
