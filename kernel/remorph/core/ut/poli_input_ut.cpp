#include <kernel/remorph/core/poli_input.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/bitmap.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/system/defaults.h>

#define VAR(NAME) Y_CAT(NAME, __LINE__)

// General checking

#define PI_CHECK(input, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDump(input, false), model)

#define PI_CHECK_R(input, resolver, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDump(input, false, &resolver), model)

#define PI_CHECK_L(input, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDump(input, true), model)

#define PI_CHECK_LR(input, resolver, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDump(input, true, &resolver), model)

// Raw checking

#define PI_CHECK_LENGTH(input, model) \
    UNIT_ASSERT_EQUAL(input.GetLength(), model)

#define PI_CHECK_EXTRA_SYMBOLS(input, model) \
    UNIT_ASSERT_EQUAL(input.GetExtraSymbols(), model)

#define PI_CHECK_MAX_ALTS_COUNT(input, model) \
    UNIT_ASSERT_EQUAL(input.GetMaxAltsCount(), model)

#define PI_CHECK_ALTS_COUNTS(input, ...) \
    size_t VAR(model)[] = {0, __VA_ARGS__}; \
    UNIT_ASSERT_EQUAL(input.GetLength(), sizeof(VAR(model)) / sizeof(size_t) - 1); \
    for (size_t p = 0; p < input.GetLength(); ++p) { \
        UNIT_ASSERT_EQUAL(input.GetAltsCount(p), VAR(model)[p + 1]); \
    }

#define PI_CHECK_DATA(input, pos, ...) \
    TTestAltData VAR(model)[] = {TTestAltData(), __VA_ARGS__}; \
    UNIT_ASSERT_EQUAL(input.GetAltsCount(pos), input.GetMarks(pos).size() + 1); \
    UNIT_ASSERT_EQUAL(input.GetAltsCount(pos), sizeof(VAR(model)) / sizeof(TTestAltData) - 1); \
    for (size_t a = 0; a < input.GetAltsCount(pos); ++a) { \
        UNIT_ASSERT_EQUAL(input.GetSymbol(pos, a), VAR(model)[a + 1].Symbol); \
        UNIT_ASSERT_EQUAL(input.GetSymbolLength(pos, a), VAR(model)[a + 1].Length); \
        if (a) { \
            UNIT_ASSERT_EQUAL(input.GetLabel(pos, a), VAR(model)[a + 1].Label); \
        } \
    } \
    UNIT_ASSERT_EQUAL(input.GetMarks(pos).size(), sizeof(VAR(model)) / sizeof(TTestAltData) - 2); \
    for (size_t m = 0; m < input.GetMarks(pos).size(); ++m) { \
        UNIT_ASSERT_EQUAL(input.GetMarks(pos)[m].Symbol, VAR(model)[m + 2].Symbol); \
        UNIT_ASSERT_EQUAL(input.GetMarks(pos)[m].Length, VAR(model)[m + 2].Length); \
        UNIT_ASSERT_EQUAL(input.GetMarks(pos)[m].Label, VAR(model)[m + 2].Label); \
    } \

// Guts checking

#define PI_CHECK_MASTER(input, seq) \
    UNIT_ASSERT_EQUAL(input.GetMaster(), TVector<char>(seq.data(), seq.data() + seq.size()))

#define PI_CHECK_ALL_LABELS(input, ...) \
    size_t VAR(modelArr)[] = {0, __VA_ARGS__}; \
    TDynBitMap VAR(model); \
    for (size_t i = 0; i < sizeof(VAR(modelArr)) / sizeof(size_t) - 1; ++i) { \
        VAR(model).Set(VAR(modelArr)[i + 1]); \
    } \
    UNIT_ASSERT((input.GetAllLabels() ^ VAR(model)).Empty())

#define PI_CHECK_LABELS(input, pos, ...) \
    size_t VAR(modelArr)[] = {0, __VA_ARGS__}; \
    TDynBitMap VAR(model); \
    for (size_t i = 0; i < sizeof(VAR(modelArr)) / sizeof(size_t) - 1; ++i) { \
        VAR(model).Set(VAR(modelArr)[i + 1]); \
    } \
    UNIT_ASSERT((input.GetLabels(pos) ^ VAR(model)).Empty())

// Symbols traversal checking

#define PI_CHECK_TRAVERSE_SYMBOLS_CO(input, offset, cut, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDumpSymbols(input, offset, cut), model)
#define PI_CHECK_TRAVERSE_SYMBOLS_C(input, cut, model) \
    PI_CHECK_TRAVERSE_SYMBOLS_CO(input, 0, cut, model)

#define PI_CHECK_TRAVERSE_SYMBOLS_O(input, offset, model) \
    PI_CHECK_TRAVERSE_SYMBOLS_CO(input, offset, 0, model)
#define PI_CHECK_TRAVERSE_SYMBOLS(input, model) \
    PI_CHECK_TRAVERSE_SYMBOLS_O(input, 0, model)

#define PI_CHECK_TRAVERSE_SYMBOLS_OR(input, offset, resolver, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDumpSymbols(input, offset, 0, &resolver), model)
#define PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, model) \
    PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 0, resolver, model)

// Branches traversal checking

#define PI_CHECK_TRAVERSE_BRANCHES_CO(input, offset, cutSymbols, cutBranches, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDumpBranches(input, offset, cutSymbols, cutBranches), model)
#define PI_CHECK_TRAVERSE_BRANCHES_C(input, cutSymbols, cutBranches, model) \
    PI_CHECK_TRAVERSE_BRANCHES_CO(input, 0, cutSymbols, cutBranches, model)

#define PI_CHECK_TRAVERSE_BRANCHES_O(input, offset, model) \
    PI_CHECK_TRAVERSE_BRANCHES_CO(input, offset, 0, 0, model)
#define PI_CHECK_TRAVERSE_BRANCHES(input, model) \
    PI_CHECK_TRAVERSE_BRANCHES_O(input, 0, model)

#define PI_CHECK_TRAVERSE_BRANCHES_OR(input, offset, resolver, model) \
    UNIT_ASSERT_STRINGS_EQUAL(TestDumpBranches(input, offset, 0, 0, &resolver), model)
#define PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, model) \
    PI_CHECK_TRAVERSE_BRANCHES_OR(input, 0, resolver, model)

// Branch track-based traversal checking

#define PI_CHECK_TRAVERSE_BRANCH_CO(input, offset, cut, model, ...) \
    size_t VAR(trackArr)[] = {0, __VA_ARGS__}; \
    TVector<size_t> VAR(track)(VAR(trackArr) + 1, VAR(trackArr) + sizeof(VAR(trackArr)) / sizeof(size_t)); \
    UNIT_ASSERT_STRINGS_EQUAL(TestDumpBranch(input, VAR(track), offset, cut), model)
#define PI_CHECK_TRAVERSE_BRANCH_C(input, cut, model, ...) \
    PI_CHECK_TRAVERSE_BRANCH_CO(input, 0, cut, model, __VA_ARGS__)

#define PI_CHECK_TRAVERSE_BRANCH_O(input, offset, model, ...) \
    PI_CHECK_TRAVERSE_BRANCH_CO(input, offset, 0, model, __VA_ARGS__)
#define PI_CHECK_TRAVERSE_BRANCH(input, model, ...) \
    PI_CHECK_TRAVERSE_BRANCH_O(input, 0, model, __VA_ARGS__)

using namespace NRemorph;

namespace {

struct TTestInput: public TPoliInput<char> {
    TTestInput(const TStringBuf& str)
        : TPoliInput<char>(TVector<char>(str.data(), str.data() + str.size()))
    {
    }
};

struct TTestAltData {
    char Symbol;
    size_t Length;
    size_t Label;
};

struct TTestResolver {
    TVector<TDynBitMap> Disabled;

    inline void operator ()(TDynBitMap& alts, size_t pos, char masterSymbol, const TVector<TTestInput::TMark>& marks,
                            const TDynBitMap& labels) const {
        Y_UNUSED(masterSymbol);
        Y_UNUSED(marks);
        Y_UNUSED(labels);
        if (pos < Disabled.size()) {
            alts -= Disabled[pos];
        }
    }

    inline void Disable(size_t pos, size_t alt) {
        if (pos >= Disabled.size()) {
            Disabled.resize(pos + 1);
        }
        Disabled[pos].Set(alt);
    }
};

struct TTestSymbolAcceptor {
    TSet<char> Symbols;

    inline bool operator ()(char symbol) const {
        return Symbols.contains(symbol);
    }

    inline void Add(char symbol) {
        Symbols.insert(symbol);
    }

    inline void Remove(char symbol) {
        Symbols.erase(symbol);
    }
};

template <typename TResolver = TTestResolver>
inline TString TestDump(const TTestInput& input, bool labeled, TResolver* resolver = nullptr) {
    struct TDumper {
        IOutputStream& Output;
        bool Labeled;
        size_t Pos;
        char CachedSymbol;
        size_t CachedSymbolLength;
        size_t* CachedLabel;
        bool Started;
        bool InSet;

        TDumper(IOutputStream& output, bool labeled2)
            : Output(output)
            , Labeled(labeled2)
            , Pos()
            , CachedSymbol()
            , CachedSymbolLength()
            , CachedLabel()
            , Started()
            , InSet()
        {
        }

        inline void Begin(size_t length, size_t extraSymbols) {
            Y_UNUSED(length);
            Y_UNUSED(extraSymbols);
            CachedSymbol = 0;
            Started = false;
            InSet = false;
        }

        inline void Position(size_t pos) {
            if (Started) {
                FlushSet();
            } else {
                Started = true;
            }
            Pos = pos;
        }

        inline void Symbol(char symbol, size_t symbolLength, size_t* label) {
            if (!InSet) {
                if (!CachedSymbol) {
                    CachedSymbol = symbol;
                    CachedSymbolLength = symbolLength;
                    CachedLabel = label;
                    return;
                }
                InSet = true;
                Output << '[';
                OutputSymbol(CachedSymbol, CachedSymbolLength, CachedLabel);
                CachedSymbol = 0;
            }
            OutputSymbol(symbol, symbolLength, label);
        }

        inline void End() {
            FlushSet();
        }

        inline void OutputSymbol(char symbol, size_t symbolLength, size_t* label) const {
            Output << symbol;
            if (symbolLength > 1) {
                Output << '(' << symbolLength << ')';
            }
            if (Labeled && label) {
                Output << '{' << *label << '}';
            }
        }

        inline void FlushSet() {
            if (InSet) {
                InSet = false;
                Output << ']';
            } else if (CachedSymbol) {
                OutputSymbol(CachedSymbol, CachedSymbolLength, CachedLabel);
                CachedSymbol = 0;
            } else {
                Output << '-';
            }
        }
    };

    TStringStream stream;
    TDumper dumper(stream, labeled);
    if (resolver) {
        input.TraverseRaw(dumper, *resolver);
    } else {
        input.TraverseRaw(dumper);
    }
    return stream.Str();
}

template <typename TResolver = TTestResolver>
inline TString TestDumpSymbols(const TTestInput& input, size_t offset, size_t cut, TResolver* resolver = nullptr) {
    struct TDumper {
        IOutputStream& Output;
        size_t Cut;
        size_t Count;

        TDumper(IOutputStream& output, size_t cut2)
            : Output(output)
            , Cut(cut2)
            , Count()
        {
        }

        inline void Begin(size_t symbols) {
            Y_UNUSED(symbols);
            Count = 0;
        }

        inline bool operator()(char symbol) {
            Output << symbol;
            ++Count;
            return !(Cut && (Count == Cut));
        }

        inline void End() {
        }
    };

    TStringStream stream;
    TDumper dumper(stream, cut);
    if (resolver) {
        input.TraverseSymbols(dumper, offset, *resolver);
    } else {
        input.TraverseSymbols(dumper, offset);
    }
    return stream.Str();
}

template <typename TResolver = TTestResolver>
inline TString TestDumpBranches(const TTestInput& input, size_t offset, size_t cutSymbols, size_t cutBranches,
                               TResolver* resolver = nullptr) {
    struct TDumper {
        IOutputStream& Output;
        TVector<size_t> Track;
        bool Separator;
        size_t CutSymbols;
        size_t CutBranches;
        size_t CountBranches;

        TDumper(IOutputStream& output, size_t cutSymbols2, size_t cutBranches2)
            : Output(output)
            , Separator()
            , CutSymbols(cutSymbols2)
            , CutBranches(cutBranches2)
            , CountBranches()
        {
        }

        inline void Begin(size_t length) {
            Track.reserve(length);
            Track.clear();
            Separator = false;
            CountBranches = 0;
        }

        inline bool Symbol(char symbol, size_t alt) {
            if (Separator) {
                Separator = false;
                Output << '|';
            }
            Output << symbol;
            Track.push_back(alt);
            return !(CutSymbols && (Track.size() == CutSymbols));
        }

        inline bool Branch() {
            Output << '<';
            if (Track) {
                Output << Track[0];
                for (size_t a = 1; a < Track.size(); ++a) {
                    Output << ',' << Track[a];
                }
            }
            Output << '>';
            Track.clear();
            Separator = true;
            ++CountBranches;
            return !(CutBranches && (CountBranches == CutBranches));
        }

        inline void End() {
        }
    };

    TStringStream stream;
    TDumper dumper(stream, cutSymbols, cutBranches);
    if (resolver) {
        input.TraverseBranches(dumper, offset, *resolver);
    } else {
        input.TraverseBranches(dumper, offset);
    }
    return stream.Str();
}

inline TString TestDumpBranch(const TTestInput& input, const TVector<size_t>& track, size_t offset, size_t cut) {
    struct TDumper {
        IOutputStream& Output;
        size_t Cut;
        size_t Count;

        TDumper(IOutputStream& output, size_t cut2)
            : Output(output)
            , Cut(cut2)
            , Count()
        {
        }

        inline void Begin(size_t length) {
            Y_UNUSED(length);
            Count = 0;
        }

        inline bool operator ()(char symbol) {
            Output << symbol;
            ++Count;
            return !(Cut && (Count == Cut));
        }

        inline void End() {
        }
    };

    TStringStream stream;
    TDumper dumper(stream, cut);
    input.TraverseBranch(dumper, track, offset);
    return stream.Str();
}

}

Y_UNIT_TEST_SUITE(RemorphCorePoliInput) {
    Y_UNIT_TEST(Empty) {
        TTestInput input("");
        PI_CHECK(input, "");
    }

    Y_UNIT_TEST(Simple) {
        TTestInput input("abcde");
        PI_CHECK(input, "abcde");
    }

    Y_UNIT_TEST(Alter) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'B'), 1);
        PI_CHECK(input, "a[bB]cde");
    }

    Y_UNIT_TEST(AlterAll) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(0, 'A'), 1);
        UNIT_ASSERT_EQUAL(input.Alter(2, 'C'), 1);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'B'), 1);
        UNIT_ASSERT_EQUAL(input.Alter(4, 'E'), 1);
        UNIT_ASSERT_EQUAL(input.Alter(3, 'D'), 1);
        PI_CHECK(input, "[aA][bB][cC][dD][eE]");
    }

    Y_UNIT_TEST(AlterMulti) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'f'), 1);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'g'), 2);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'h'), 3);
        PI_CHECK(input, "a[bfgh]cde");
    }

    Y_UNIT_TEST(AlterLong) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'f', 1), 1);
        PI_CHECK(input, "a[bf]cde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'g', 2), 2);
        PI_CHECK(input, "a[bfg(2)]cde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'h', 3), 3);
        PI_CHECK(input, "a[bfg(2)h(3)]cde");
    }

    Y_UNIT_TEST(AlterWhole) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(0, 'f', 5), 1);
        PI_CHECK(input, "[af(5)]bcde");
    }

    Y_UNIT_TEST(AlterLabeled) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'f', 1, 0), 1);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'g', 1, 1), 2);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'h', 1, 32), 3);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'i', 1, 15), 4);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'j', 1, 1), 5);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'k', 3, 32), 6);
        UNIT_ASSERT_EQUAL(input.Alter(2, 'l', 3, 32), 1);
        UNIT_ASSERT_EQUAL(input.Alter(1, 'm', 1), 7);
        PI_CHECK_L(input, "a[bf{0}g{1}h{32}i{15}j{1}k(3){32}m{0}][cl(3){32}]de");
    }

    Y_UNIT_TEST(ResolveEmpty) {
        TTestInput input("");
        TTestResolver resolver;
        PI_CHECK_R(input, resolver, "");
        resolver.Disable(0, 0);
        PI_CHECK_R(input, resolver, "");
        resolver.Disable(1, 1);
        PI_CHECK_R(input, resolver, "");
    }

    Y_UNIT_TEST(ResolveSimple) {
        TTestInput input("abcde");
        TTestResolver resolver;
        PI_CHECK_R(input, resolver, "abcde");
        resolver.Disable(1, 1);
        PI_CHECK_R(input, resolver, "abcde");
        resolver.Disable(3, 0);
        PI_CHECK_R(input, resolver, "abc-e");
        resolver.Disable(0, 0);
        PI_CHECK_R(input, resolver, "-bc-e");
        resolver.Disable(1, 0);
        resolver.Disable(2, 0);
        resolver.Disable(4, 0);
        PI_CHECK_R(input, resolver, "-----");
    }

    Y_UNIT_TEST(ResolveMulti) {
        TTestInput input("abcde");

        TTestResolver resolver1;
        resolver1.Disable(2, 0);
        PI_CHECK_R(input, resolver1, "ab-de");

        TTestResolver resolver2;
        resolver2.Disable(3, 0);
        PI_CHECK_R(input, resolver2, "abc-e");
        PI_CHECK_R(input, resolver1, "ab-de");

        TTestResolver resolver3;
        PI_CHECK_R(input, resolver3, "abcde");
    }

    Y_UNIT_TEST(ResolveAltered) {
        TTestInput input("abcde");
        input.Alter(1, 'B', 3);

        TTestResolver resolver;
        resolver.Disable(1, 2);
        PI_CHECK_R(input, resolver, "a[bB(3)]cde");
        resolver.Disable(3, 0);
        PI_CHECK_R(input, resolver, "a[bB(3)]c-e");
        resolver.Disable(2, 0);
        PI_CHECK_R(input, resolver, "a[bB(3)]--e");
        resolver.Disable(4, 0);
        PI_CHECK_R(input, resolver, "a[bB(3)]---");
        resolver.Disable(0, 0);
        PI_CHECK_R(input, resolver, "-[bB(3)]---");

        TTestResolver withoutBAlt;
        withoutBAlt.Disable(1, 1);
        PI_CHECK_R(input, withoutBAlt, "abcde");

        TTestResolver withoutBOrig;
        withoutBOrig.Disable(1, 0);
        PI_CHECK_R(input, withoutBOrig, "aB(3)cde");
    }

    Y_UNIT_TEST(ResolveAlteredLabeled) {
        TTestInput input("abcde");
        input.Alter(1, 'B', 3, 7);

        TTestResolver withoutBAlt;
        withoutBAlt.Disable(1, 1);
        PI_CHECK_LR(input, withoutBAlt, "abcde");

        TTestResolver withoutBOrig;
        withoutBOrig.Disable(1, 0);
        PI_CHECK_LR(input, withoutBOrig, "aB(3){7}cde");
    }

    Y_UNIT_TEST(RawEmpty) {
        TTestInput input("");
        PI_CHECK_LENGTH(input, 0);
        PI_CHECK_EXTRA_SYMBOLS(input, 0);
        PI_CHECK_ALTS_COUNTS(input);
        PI_CHECK_MAX_ALTS_COUNT(input, 0);
    }

    Y_UNIT_TEST(RawSimple) {
        TTestInput input("abcde");
        PI_CHECK_LENGTH(input, 5);
        PI_CHECK_EXTRA_SYMBOLS(input, 0);
        PI_CHECK_ALTS_COUNTS(input, 1, 1, 1, 1, 1);
        PI_CHECK_MAX_ALTS_COUNT(input, 1);
        PI_CHECK_DATA(input, 0, {'a', 1, 0});
        PI_CHECK_DATA(input, 1, {'b', 1, 0});
        PI_CHECK_DATA(input, 2, {'c', 1, 0});
        PI_CHECK_DATA(input, 3, {'d', 1, 0});
        PI_CHECK_DATA(input, 4, {'e', 1, 0});
    }

    Y_UNIT_TEST(RawAltered) {
        TTestInput input("abcde");

        input.Alter(1, 'B', 3);
        PI_CHECK_LENGTH(input, 5);
        PI_CHECK_EXTRA_SYMBOLS(input, 1);
        PI_CHECK_ALTS_COUNTS(input, 1, 2, 1, 1, 1);
        PI_CHECK_MAX_ALTS_COUNT(input, 2);
        PI_CHECK_DATA(input, 0, {'a', 1, 0});
        PI_CHECK_DATA(input, 1, {'b', 1, 0}, {'B', 3, 0});
        PI_CHECK_DATA(input, 2, {'c', 1, 0});
        PI_CHECK_DATA(input, 3, {'d', 1, 0});
        PI_CHECK_DATA(input, 4, {'e', 1, 0});

        input.Alter(2, 'C', 2);
        input.Alter(2, 'f', 3);
        PI_CHECK_LENGTH(input, 5);
        PI_CHECK_EXTRA_SYMBOLS(input, 3);
        PI_CHECK_ALTS_COUNTS(input, 1, 2, 3, 1, 1);
        PI_CHECK_MAX_ALTS_COUNT(input, 3);
        PI_CHECK_DATA(input, 0, {'a', 1, 0});
        PI_CHECK_DATA(input, 1, {'b', 1, 0}, {'B', 3, 0});
        PI_CHECK_DATA(input, 2, {'c', 1, 0}, {'C', 2, 0}, {'f', 3, 0});
        PI_CHECK_DATA(input, 3, {'d', 1, 0});
        PI_CHECK_DATA(input, 4, {'e', 1, 0});
    }

    Y_UNIT_TEST(RawAlteredLabeled) {
        TTestInput input("abcde");

        input.Alter(1, 'B', 3, 7);
        PI_CHECK_LENGTH(input, 5);
        PI_CHECK_EXTRA_SYMBOLS(input, 1);
        PI_CHECK_ALTS_COUNTS(input, 1, 2, 1, 1, 1);
        PI_CHECK_MAX_ALTS_COUNT(input, 2);
        PI_CHECK_DATA(input, 0, {'a', 1, 0});
        PI_CHECK_DATA(input, 1, {'b', 1, 0}, {'B', 3, 7});
        PI_CHECK_DATA(input, 2, {'c', 1, 0});
        PI_CHECK_DATA(input, 3, {'d', 1, 0});
        PI_CHECK_DATA(input, 4, {'e', 1, 0});

        input.Alter(2, 'C', 2, 8);
        input.Alter(2, 'f', 3, 15);
        PI_CHECK_LENGTH(input, 5);
        PI_CHECK_EXTRA_SYMBOLS(input, 3);
        PI_CHECK_ALTS_COUNTS(input, 1, 2, 3, 1, 1);
        PI_CHECK_MAX_ALTS_COUNT(input, 3);
        PI_CHECK_DATA(input, 0, {'a', 1, 0});
        PI_CHECK_DATA(input, 1, {'b', 1, 0}, {'B', 3, 7});
        PI_CHECK_DATA(input, 2, {'c', 1, 0}, {'C', 2, 8}, {'f', 3, 15});
        PI_CHECK_DATA(input, 3, {'d', 1, 0});
        PI_CHECK_DATA(input, 4, {'e', 1, 0});
    }

    Y_UNIT_TEST(GutsEmpty) {
        TStringBuf seq = TStringBuf();
        TTestInput input(seq);
        PI_CHECK_MASTER(input, seq);
    }

    Y_UNIT_TEST(GutsSimple) {
        TStringBuf seq = "abcde";
        TTestInput input(seq);
        PI_CHECK_MASTER(input, seq);
        PI_CHECK_ALL_LABELS(input);
        PI_CHECK_LABELS(input, 0);
        PI_CHECK_LABELS(input, 1);
        PI_CHECK_LABELS(input, 2);
        PI_CHECK_LABELS(input, 3);
        PI_CHECK_LABELS(input, 4);
    }

    Y_UNIT_TEST(GutsAltered) {
        TStringBuf seq = "abcde";
        TTestInput input(seq);

        input.Alter(1, 'B', 3);
        PI_CHECK_MASTER(input, seq);
        PI_CHECK_ALL_LABELS(input, 0);
        PI_CHECK_LABELS(input, 0);
        PI_CHECK_LABELS(input, 1, 0);
        PI_CHECK_LABELS(input, 2);
        PI_CHECK_LABELS(input, 3);
        PI_CHECK_LABELS(input, 4);

        input.Alter(2, 'C', 2);
        input.Alter(2, 'f', 3);
        PI_CHECK_MASTER(input, seq);
        PI_CHECK_ALL_LABELS(input, 0);
        PI_CHECK_LABELS(input, 0);
        PI_CHECK_LABELS(input, 1, 0);
        PI_CHECK_LABELS(input, 2, 0);
        PI_CHECK_LABELS(input, 3);
        PI_CHECK_LABELS(input, 4);
    }

    Y_UNIT_TEST(GutsAlteredLabeled) {
        TStringBuf seq = "abcde";
        TTestInput input(seq);

        input.Alter(1, 'B', 3, 7);
        PI_CHECK_MASTER(input, seq);
        PI_CHECK_ALL_LABELS(input, 7);
        PI_CHECK_LABELS(input, 0);
        PI_CHECK_LABELS(input, 1, 7);
        PI_CHECK_LABELS(input, 2);
        PI_CHECK_LABELS(input, 3);
        PI_CHECK_LABELS(input, 4);

        input.Alter(2, 'C', 2, 8);
        input.Alter(2, 'f', 3, 15);
        PI_CHECK_MASTER(input, seq);
        PI_CHECK_ALL_LABELS(input, 7, 8, 15);
        PI_CHECK_LABELS(input, 0);
        PI_CHECK_LABELS(input, 1, 7);
        PI_CHECK_LABELS(input, 2, 8, 15);
        PI_CHECK_LABELS(input, 3);
        PI_CHECK_LABELS(input, 4);
    }

    Y_UNIT_TEST(TraverseSymbolsEmpty) {
        TTestInput input("");
        PI_CHECK_TRAVERSE_SYMBOLS(input, "");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input, 0, "");
    }

    Y_UNIT_TEST(TraverseSymbolsSimple) {
        TTestInput input("abcde");
        PI_CHECK_TRAVERSE_SYMBOLS(input, "abcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input, 0, "abcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input, 2, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input, 5, "");
    }

    Y_UNIT_TEST(TraverseSymbolsAltered) {
        TTestInput input1("abcde");
        input1.Alter(1, 'B');
        PI_CHECK_TRAVERSE_SYMBOLS(input1, "abBcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input1, 0, "abBcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input1, 1, "bBcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input1, 2, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input1, 5, "");

        TTestInput input2("abcde");
        input2.Alter(0, 'A');
        input2.Alter(2, 'C');
        input2.Alter(1, 'B');
        input2.Alter(4, 'E');
        input2.Alter(3, 'D');
        PI_CHECK_TRAVERSE_SYMBOLS(input2, "aAbBcCdDeE");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input2, 2, "cCdDeE");

        TTestInput input3("abcde");
        input3.Alter(1, 'f');
        input3.Alter(1, 'g');
        input3.Alter(1, 'h');
        PI_CHECK_TRAVERSE_SYMBOLS(input3, "abfghcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input3, 1, "bfghcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input3, 2, "cde");

        TTestInput input4("abcde");
        input4.Alter(1, 'f', 1);
        PI_CHECK_TRAVERSE_SYMBOLS(input4, "abfcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input4, 1, "bfcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input4, 2, "cde");
        input4.Alter(1, 'g', 2);
        PI_CHECK_TRAVERSE_SYMBOLS(input4, "abfgcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input4, 1, "bfgcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input4, 2, "cde");
        input4.Alter(1, 'h', 3);
        PI_CHECK_TRAVERSE_SYMBOLS(input4, "abfghcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input4, 1, "bfghcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input4, 2, "cde");

        TTestInput input5("abcde");
        input5.Alter(0, 'f', 5);
        PI_CHECK_TRAVERSE_SYMBOLS(input5, "afbcde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input5, 1, "bcde");

        TTestInput input6("abcde");
        input6.Alter(1, 'f', 1, 0);
        input6.Alter(1, 'g', 1, 1);
        input6.Alter(1, 'h', 1, 32);
        input6.Alter(1, 'i', 1, 15);
        input6.Alter(1, 'j', 1, 1);
        input6.Alter(1, 'k', 3, 32);
        input6.Alter(2, 'l', 3, 32);
        input6.Alter(1, 'm', 1);
        PI_CHECK_TRAVERSE_SYMBOLS(input6, "abfghijkmclde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input6, 1, "bfghijkmclde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input6, 2, "clde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input6, 3, "de");
    }

    Y_UNIT_TEST(TraverseSymbolsResolved) {
        TTestInput input1("");
        TTestResolver resolver1;
        PI_CHECK_TRAVERSE_SYMBOLS_R(input1, resolver1, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input1, 0, resolver1, "");

        TTestInput input2("abcde");
        TTestResolver resolver2;
        PI_CHECK_TRAVERSE_SYMBOLS_R(input2, resolver2, "abcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input2, 0, resolver2, "abcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input2, 2, resolver2, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input2, 5, resolver2, "");
        resolver2.Disable(3, 0);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input2, resolver2, "abc");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input2, 0, resolver2, "abc");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input2, 2, resolver2, "c");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input2, 3, resolver2, "");
        resolver2.Disable(0, 0);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input2, resolver2, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input2, 0, resolver2, "");

        TTestInput input3("abcde");
        input3.Alter(1, 'B', 3);

        TTestResolver resolver3;
        PI_CHECK_TRAVERSE_SYMBOLS_R(input3, resolver3, "abBcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 0, resolver3, "abBcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 1, resolver3, "bBcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 2, resolver3, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 3, resolver3, "de");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 4, resolver3, "e");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 5, resolver3, "");
        resolver3.Disable(3, 0);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input3, resolver3, "abBce");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 1, resolver3, "bBce");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 2, resolver3, "c");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 4, resolver3, "e");
        resolver3.Disable(2, 0);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input3, resolver3, "abBe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 1, resolver3, "bBe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 2, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 4, resolver3, "e");
        resolver3.Disable(4, 0);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input3, resolver3, "abB");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 1, resolver3, "bB");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 2, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 4, resolver3, "");
        resolver3.Disable(0, 0);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input3, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 1, resolver3, "bB");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 2, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 4, resolver3, "");

        TTestResolver withoutBAlt;
        withoutBAlt.Disable(1, 1);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input3, withoutBAlt, "abcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 1, withoutBAlt, "bcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 2, withoutBAlt, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 3, withoutBAlt, "de");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 4, withoutBAlt, "e");

        TTestResolver withoutBOrig;
        withoutBOrig.Disable(1, 0);
        PI_CHECK_TRAVERSE_SYMBOLS_R(input3, withoutBOrig, "aBe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 1, withoutBOrig, "Be");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 2, withoutBOrig, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 3, withoutBOrig, "de");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input3, 4, withoutBOrig, "e");
    }

    Y_UNIT_TEST(TraverseSymbolsCut) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        PI_CHECK_TRAVERSE_SYMBOLS_C(input, 0, "abfgcdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_CO(input, 2, 0, "cdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_C(input, 8, "abfgcdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_CO(input, 2, 4, "cdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_C(input, 3, "abf");
        PI_CHECK_TRAVERSE_SYMBOLS_CO(input, 2, 3, "cdh");
    }

    Y_UNIT_TEST(TraverseBranchesEmpty) {
        TTestInput input("");
        PI_CHECK_TRAVERSE_BRANCHES(input, "");
        PI_CHECK_TRAVERSE_BRANCHES_O(input, 0, "");
    }

    Y_UNIT_TEST(TraverseBranchesSimple) {
        TTestInput input("abcde");
        PI_CHECK_TRAVERSE_BRANCHES(input, "abcde<0,0,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input, 0, "abcde<0,0,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input, 2, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input, 5, "");
    }

    Y_UNIT_TEST(TraverseBranchesAltered) {
        TTestInput input1("abcde");
        input1.Alter(1, 'B');
        PI_CHECK_TRAVERSE_BRANCHES(input1, "abcde<0,0,0,0,0>|"
                                           "aBcde<0,1,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input1, 0, "abcde<0,0,0,0,0>|"
                                                "aBcde<0,1,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input1, 1, "bcde<0,0,0,0>|"
                                                "Bcde<1,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input1, 2, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input1, 5, "");

        TTestInput input2("abcde");
        input2.Alter(0, 'A');
        input2.Alter(2, 'C');
        input2.Alter(1, 'B');
        input2.Alter(4, 'E');
        input2.Alter(3, 'D');
        PI_CHECK_TRAVERSE_BRANCHES(input2, "abcde<0,0,0,0,0>|"
                                           "abcdE<0,0,0,0,1>|"
                                           "abcDe<0,0,0,1,0>|"
                                           "abcDE<0,0,0,1,1>|"
                                           "abCde<0,0,1,0,0>|"
                                           "abCdE<0,0,1,0,1>|"
                                           "abCDe<0,0,1,1,0>|"
                                           "abCDE<0,0,1,1,1>|"
                                           "aBcde<0,1,0,0,0>|"
                                           "aBcdE<0,1,0,0,1>|"
                                           "aBcDe<0,1,0,1,0>|"
                                           "aBcDE<0,1,0,1,1>|"
                                           "aBCde<0,1,1,0,0>|"
                                           "aBCdE<0,1,1,0,1>|"
                                           "aBCDe<0,1,1,1,0>|"
                                           "aBCDE<0,1,1,1,1>|"
                                           "Abcde<1,0,0,0,0>|"
                                           "AbcdE<1,0,0,0,1>|"
                                           "AbcDe<1,0,0,1,0>|"
                                           "AbcDE<1,0,0,1,1>|"
                                           "AbCde<1,0,1,0,0>|"
                                           "AbCdE<1,0,1,0,1>|"
                                           "AbCDe<1,0,1,1,0>|"
                                           "AbCDE<1,0,1,1,1>|"
                                           "ABcde<1,1,0,0,0>|"
                                           "ABcdE<1,1,0,0,1>|"
                                           "ABcDe<1,1,0,1,0>|"
                                           "ABcDE<1,1,0,1,1>|"
                                           "ABCde<1,1,1,0,0>|"
                                           "ABCdE<1,1,1,0,1>|"
                                           "ABCDe<1,1,1,1,0>|"
                                           "ABCDE<1,1,1,1,1>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input2, 2, "cde<0,0,0>|"
                                                "cdE<0,0,1>|"
                                                "cDe<0,1,0>|"
                                                "cDE<0,1,1>|"
                                                "Cde<1,0,0>|"
                                                "CdE<1,0,1>|"
                                                "CDe<1,1,0>|"
                                                "CDE<1,1,1>");

        TTestInput input3("abcde");
        input3.Alter(1, 'f');
        input3.Alter(1, 'g');
        input3.Alter(1, 'h');
        PI_CHECK_TRAVERSE_BRANCHES(input3, "abcde<0,0,0,0,0>|"
                                           "afcde<0,1,0,0,0>|"
                                           "agcde<0,2,0,0,0>|"
                                           "ahcde<0,3,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input3, 1, "bcde<0,0,0,0>|"
                                                "fcde<1,0,0,0>|"
                                                "gcde<2,0,0,0>|"
                                                "hcde<3,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input3, 2, "cde<0,0,0>");

        TTestInput input4("abcde");
        input4.Alter(1, 'f', 1);
        PI_CHECK_TRAVERSE_BRANCHES(input4, "abcde<0,0,0,0,0>|"
                                           "afcde<0,1,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input4, 1, "bcde<0,0,0,0>|"
                                                "fcde<1,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input4, 2, "cde<0,0,0>");
        input4.Alter(1, 'g', 2);
        PI_CHECK_TRAVERSE_BRANCHES(input4, "abcde<0,0,0,0,0>|"
                                           "afcde<0,1,0,0,0>|"
                                           "agde<0,2,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input4, 1, "bcde<0,0,0,0>|"
                                                "fcde<1,0,0,0>|"
                                                "gde<2,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input4, 2, "cde<0,0,0>");
        input4.Alter(1, 'h', 3);
        PI_CHECK_TRAVERSE_BRANCHES(input4, "abcde<0,0,0,0,0>|"
                                           "afcde<0,1,0,0,0>|"
                                           "agde<0,2,0,0>|"
                                           "ahe<0,3,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input4, 1, "bcde<0,0,0,0>|"
                                                "fcde<1,0,0,0>|"
                                                "gde<2,0,0>|"
                                                "he<3,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input4, 2, "cde<0,0,0>");

        TTestInput input5("abcde");
        input5.Alter(0, 'f', 5);
        PI_CHECK_TRAVERSE_BRANCHES(input5, "abcde<0,0,0,0,0>|"
                                           "f<1>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input5, 1, "bcde<0,0,0,0>");

        TTestInput input6("abcde");
        input6.Alter(1, 'f', 1, 0);
        input6.Alter(1, 'g', 1, 1);
        input6.Alter(1, 'h', 1, 32);
        input6.Alter(1, 'i', 1, 15);
        input6.Alter(1, 'j', 1, 1);
        input6.Alter(1, 'k', 3, 32);
        input6.Alter(2, 'l', 3, 32);
        input6.Alter(1, 'm', 1);
        PI_CHECK_TRAVERSE_BRANCHES(input6, "abcde<0,0,0,0,0>|"
                                           "abl<0,0,1>|"
                                           "afcde<0,1,0,0,0>|"
                                           "afl<0,1,1>|"
                                           "agcde<0,2,0,0,0>|"
                                           "agl<0,2,1>|"
                                           "ahcde<0,3,0,0,0>|"
                                           "ahl<0,3,1>|"
                                           "aicde<0,4,0,0,0>|"
                                           "ail<0,4,1>|"
                                           "ajcde<0,5,0,0,0>|"
                                           "ajl<0,5,1>|"
                                           "ake<0,6,0>|"
                                           "amcde<0,7,0,0,0>|"
                                           "aml<0,7,1>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input6, 1, "bcde<0,0,0,0>|"
                                                "bl<0,1>|"
                                                "fcde<1,0,0,0>|"
                                                "fl<1,1>|"
                                                "gcde<2,0,0,0>|"
                                                "gl<2,1>|"
                                                "hcde<3,0,0,0>|"
                                                "hl<3,1>|"
                                                "icde<4,0,0,0>|"
                                                "il<4,1>|"
                                                "jcde<5,0,0,0>|"
                                                "jl<5,1>|"
                                                "ke<6,0>|"
                                                "mcde<7,0,0,0>|"
                                                "ml<7,1>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input6, 2, "cde<0,0,0>|"
                                                "l<1>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input6, 3, "de<0,0>");
    }

    Y_UNIT_TEST(TraverseBranchesResolved) {
        TTestInput input1("");
        TTestResolver resolver1;
        PI_CHECK_TRAVERSE_BRANCHES_R(input1, resolver1, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input1, 0, resolver1, "");

        TTestInput input2("abcde");
        TTestResolver resolver2;
        PI_CHECK_TRAVERSE_BRANCHES_R(input2, resolver2, "abcde<0,0,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input2, 0, resolver2, "abcde<0,0,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input2, 2, resolver2, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input2, 5, resolver2, "");
        resolver2.Disable(3, 0);
        PI_CHECK_TRAVERSE_BRANCHES_R(input2, resolver2, "abc<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input2, 0, resolver2, "abc<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input2, 2, resolver2, "c<0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input2, 3, resolver2, "");
        resolver2.Disable(0, 0);
        PI_CHECK_TRAVERSE_BRANCHES_R(input2, resolver2, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input2, 0, resolver2, "");

        TTestInput input3("abcde");
        input3.Alter(1, 'B', 3);

        TTestResolver resolver3;
        PI_CHECK_TRAVERSE_BRANCHES_R(input3, resolver3, "abcde<0,0,0,0,0>|"
                                                        "aBe<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 0, resolver3, "abcde<0,0,0,0,0>|"
                                                            "aBe<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 1, resolver3, "bcde<0,0,0,0>|"
                                                            "Be<1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 2, resolver3, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 3, resolver3, "de<0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 4, resolver3, "e<0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 5, resolver3, "");
        resolver3.Disable(3, 0);
        PI_CHECK_TRAVERSE_BRANCHES_R(input3, resolver3, "abc<0,0,0>|"
                                                        "aBe<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 1, resolver3, "bc<0,0>|"
                                                            "Be<1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 2, resolver3, "c<0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 4, resolver3, "e<0>");
        resolver3.Disable(2, 0);
        PI_CHECK_TRAVERSE_BRANCHES_R(input3, resolver3, "ab<0,0>|"
                                                        "aBe<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 1, resolver3, "b<0>|"
                                                            "Be<1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 2, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 4, resolver3, "e<0>");
        resolver3.Disable(4, 0);
        PI_CHECK_TRAVERSE_BRANCHES_R(input3, resolver3, "ab<0,0>|"
                                                        "aB<0,1>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 1, resolver3, "b<0>|"
                                                            "B<1>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 2, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 4, resolver3, "");
        resolver3.Disable(0, 0);
        PI_CHECK_TRAVERSE_BRANCHES_R(input3, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 1, resolver3, "b<0>|"
                                                            "B<1>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 2, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 3, resolver3, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 4, resolver3, "");

        TTestResolver withoutBAlt;
        withoutBAlt.Disable(1, 1);
        PI_CHECK_TRAVERSE_BRANCHES_R(input3, withoutBAlt, "abcde<0,0,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 1, withoutBAlt, "bcde<0,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 2, withoutBAlt, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 3, withoutBAlt, "de<0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 4, withoutBAlt, "e<0>");

        TTestResolver withoutBOrig;
        withoutBOrig.Disable(1, 0);
        PI_CHECK_TRAVERSE_BRANCHES_R(input3, withoutBOrig, "aBe<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 1, withoutBOrig, "Be<1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 2, withoutBOrig, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 3, withoutBOrig, "de<0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input3, 4, withoutBOrig, "e<0>");
    }

    Y_UNIT_TEST(TraverseBranchesCut) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        PI_CHECK_TRAVERSE_BRANCHES_C(input, 0, 0, "abcde<0,0,0,0,0>|"
                                                  "abche<0,0,0,1,0>|"
                                                  "afe<0,1,0>|"
                                                  "agde<0,2,0,0>|"
                                                  "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_CO(input, 2, 0, 0, "cde<0,0,0>|"
                                                      "che<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_C(input, 5, 5, "abcde<0,0,0,0,0>|"
                                                  "abche<0,0,0,1,0>|"
                                                  "afe<0,1,0>|"
                                                  "agde<0,2,0,0>|"
                                                  "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_CO(input, 2, 3, 2, "cde<0,0,0>|"
                                                      "che<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_C(input, 3, 0, "abc<0,0,0>|"
                                                  "afe<0,1,0>|"
                                                  "agd<0,2,0>|"
                                                  "agh<0,2,1>");
        PI_CHECK_TRAVERSE_BRANCHES_CO(input, 2, 1, 0, "c<0>");
        PI_CHECK_TRAVERSE_BRANCHES_C(input, 0, 2, "abcde<0,0,0,0,0>|"
                                                  "abche<0,0,0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_CO(input, 2, 0, 1, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_C(input, 3, 2, "abc<0,0,0>|"
                                                  "afe<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_CO(input, 2, 1, 1, "c<0>");
    }

    Y_UNIT_TEST(TraverseBranchEmpty) {
        TTestInput input("");
        PI_CHECK_TRAVERSE_BRANCH(input, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input, 0, "");
    }

    Y_UNIT_TEST(TraverseBranchSimple) {
        TTestInput input("abcde");
        PI_CHECK_TRAVERSE_BRANCH(input, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input, "abc", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input, 0, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input, 5, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input, 0, "abc", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input, 2, "c", 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input, 3, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input, 0, "");
    }

    Y_UNIT_TEST(TraverseBranchAltered) {
        TTestInput input1("abcde");
        input1.Alter(1, 'B');
        PI_CHECK_TRAVERSE_BRANCH(input1, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input1, "aBcde", 0, 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input1, "ab", 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input1, "aB", 0, 1);
        PI_CHECK_TRAVERSE_BRANCH(input1, "a", 0);
        PI_CHECK_TRAVERSE_BRANCH(input1, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 0, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 1, "bcde", 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 5, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 0, "aBcde", 0, 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 1, "Bcde", 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 0, "ab", 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 1, "b", 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 2, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 0, "aB", 0, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 1, "B", 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 0, "a", 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 1, "");
        PI_CHECK_TRAVERSE_BRANCH_O(input1, 0, "");

        TTestInput input2("abcde");
        input2.Alter(0, 'A');
        input2.Alter(2, 'C');
        input2.Alter(1, 'B');
        input2.Alter(4, 'E');
        input2.Alter(3, 'D');
        PI_CHECK_TRAVERSE_BRANCH(input2, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input2, "ABCDE", 1, 1, 1, 1, 1);
        PI_CHECK_TRAVERSE_BRANCH(input2, "aBcDe", 0, 1, 0, 1, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input2, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input2, 2, "CDE", 1, 1, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input2, 2, "cDe", 0, 1, 0);

        TTestInput input3("abcde");
        input3.Alter(1, 'f');
        input3.Alter(1, 'g');
        input3.Alter(1, 'h');
        PI_CHECK_TRAVERSE_BRANCH(input3, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input3, "afcde", 0, 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input3, "agcde", 0, 2, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input3, "ahcde", 0, 3, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input3, 1, "bcde", 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input3, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input3, 1, "fcde", 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input3, 1, "gcde", 2, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input3, 1, "hcde", 3, 0, 0, 0);

        TTestInput input4("abcde");
        input4.Alter(1, 'f', 1);
        PI_CHECK_TRAVERSE_BRANCH(input4, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input4, "afcde", 0, 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "bcde", 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "fcde", 1, 0, 0, 0);
        input4.Alter(1, 'g', 2);
        PI_CHECK_TRAVERSE_BRANCH(input4, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input4, "afcde", 0, 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input4, "agde", 0, 2, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "bcde", 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "fcde", 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "gde", 2, 0, 0);
        input4.Alter(1, 'h', 3);
        PI_CHECK_TRAVERSE_BRANCH(input4, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input4, "afcde", 0, 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input4, "agde", 0, 2, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input4, "ahe", 0, 3, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "bcde", 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "fcde", 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "gde", 2, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input4, 1, "he", 3, 0);

        TTestInput input5("abcde");
        input5.Alter(0, 'f', 5);
        PI_CHECK_TRAVERSE_BRANCH(input5, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input5, "f", 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input5, 1, "bcde", 0, 0, 0, 0);

        TTestInput input6("abcde");
        input6.Alter(1, 'f', 1, 0);
        input6.Alter(1, 'g', 1, 1);
        input6.Alter(1, 'h', 1, 32);
        input6.Alter(1, 'i', 1, 15);
        input6.Alter(1, 'j', 1, 1);
        input6.Alter(1, 'k', 3, 32);
        input6.Alter(2, 'l', 3, 32);
        input6.Alter(1, 'm', 1);
        PI_CHECK_TRAVERSE_BRANCH(input6, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "abl", 0, 0, 1);
        PI_CHECK_TRAVERSE_BRANCH(input6, "afcde", 0, 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "afl", 0, 1, 1);
        PI_CHECK_TRAVERSE_BRANCH(input6, "agcde", 0, 2, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "agl", 0, 2, 1);
        PI_CHECK_TRAVERSE_BRANCH(input6, "ahcde", 0, 3, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "ahl", 0, 3, 1);
        PI_CHECK_TRAVERSE_BRANCH(input6, "aicde", 0, 4, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "ail", 0, 4, 1);
        PI_CHECK_TRAVERSE_BRANCH(input6, "ajcde", 0, 5, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "ajl", 0, 5, 1);
        PI_CHECK_TRAVERSE_BRANCH(input6, "ake", 0, 6, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "amcde", 0, 7, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input6, "aml", 0, 7, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "bcde", 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "bl", 0, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "fcde", 1, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "fl", 1, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "gcde", 2, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "gl", 2, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "hcde", 3, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "hl", 3, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "icde", 4, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "il", 4, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "jcde", 5, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "jl", 5, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "ke", 6, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "mcde", 7, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 1, "ml", 7, 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 2, "l", 1);
        PI_CHECK_TRAVERSE_BRANCH_O(input6, 3, "de", 0, 0);
    }

    Y_UNIT_TEST(TraverseBranchCut) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        PI_CHECK_TRAVERSE_BRANCH_C(input, 0, "aghe", 0, 2, 1, 0);
        PI_CHECK_TRAVERSE_BRANCH_CO(input, 2, 0, "che", 0, 1, 0);
        PI_CHECK_TRAVERSE_BRANCH_C(input, 4, "aghe", 0, 2, 1, 0);
        PI_CHECK_TRAVERSE_BRANCH_CO(input, 2, 3, "che", 0, 1, 0);
        PI_CHECK_TRAVERSE_BRANCH_C(input, 2, "ag", 0, 2, 1, 0);
        PI_CHECK_TRAVERSE_BRANCH_CO(input, 2, 1, "c", 0, 1, 0);
    }

    Y_UNIT_TEST(ResolverGreedy) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        TTestInput::TGreedyResolver resolver;
        PI_CHECK_R(input, resolver, "a[bf(3)g(2)]c[dh]e");
        PI_CHECK_LR(input, resolver, "a[bf(3){0}g(2){1}]c[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abfgcdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "abche<0,0,0,1,0>|"
                                                      "afe<0,1,0>|"
                                                      "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>|"
                                                          "che<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");
    }

    Y_UNIT_TEST(ResolverPreset) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        TTestInput::TPresetResolver resolver;
        PI_CHECK_R(input, resolver, "-----");
        PI_CHECK_LR(input, resolver, "-----");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "");

        TDynBitMap allAlts;
        allAlts.Set(0, input.GetMaxAltsCount());
        resolver.AllowedAlts.assign(input.GetLength(), allAlts);
        PI_CHECK_R(input, resolver, "a[bf(3)g(2)]c[dh]e");
        PI_CHECK_LR(input, resolver, "a[bf(3){0}g(2){1}]c[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abfgcdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "abche<0,0,0,1,0>|"
                                                      "afe<0,1,0>|"
                                                      "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>|"
                                                          "che<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");

        resolver.AllowedAlts[2].Reset(0);
        PI_CHECK_R(input, resolver, "a[bf(3)g(2)]-[dh]e");
        PI_CHECK_LR(input, resolver, "a[bf(3){0}g(2){1}]-[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abfgdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "ab<0,0>|"
                                                      "afe<0,1,0>|"
                                                      "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");

        resolver.AllowedAlts[2].Set(0);
        resolver.AllowedAlts[1].Reset(1);
        resolver.AllowedAlts[3].Reset(1);
        PI_CHECK_R(input, resolver, "a[bg(2)]cde");
        PI_CHECK_LR(input, resolver, "a[bg(2){1}]cde");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abgcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "de");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "agde<0,2,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>");
    }

    Y_UNIT_TEST(ResolverSymbol) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        TTestInput::TSymbolResolver<TTestSymbolAcceptor> resolver;
        PI_CHECK_R(input, resolver, "-----");
        PI_CHECK_LR(input, resolver, "-----");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "");

        TStringBuf allSymbols = "abcdefgh";
        for (auto symbol: allSymbols) {
            resolver.Acceptor.Add(symbol);
        }
        PI_CHECK_R(input, resolver, "a[bf(3)g(2)]c[dh]e");
        PI_CHECK_LR(input, resolver, "a[bf(3){0}g(2){1}]c[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abfgcdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "abche<0,0,0,1,0>|"
                                                      "afe<0,1,0>|"
                                                      "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>|"
                                                          "che<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");

        resolver.Acceptor.Remove('c');
        PI_CHECK_R(input, resolver, "a[bf(3)g(2)]-[dh]e");
        PI_CHECK_LR(input, resolver, "a[bf(3){0}g(2){1}]-[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abfgdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "ab<0,0>|"
                                                      "afe<0,1,0>|"
                                                      "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");

        resolver.Acceptor.Add('c');
        resolver.Acceptor.Remove('f');
        resolver.Acceptor.Remove('h');
        PI_CHECK_R(input, resolver, "a[bg(2)]cde");
        PI_CHECK_LR(input, resolver, "a[bg(2){1}]cde");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abgcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "de");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "agde<0,2,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>");
    }

    Y_UNIT_TEST(ResolverLabel) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        TTestInput::TLabelResolver resolver;
        PI_CHECK_R(input, resolver, "abcde");
        PI_CHECK_LR(input, resolver, "abcde");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "de");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>");

        resolver.AllowedLabels = input.GetAllLabels();
        PI_CHECK_R(input, resolver, "a[bf(3)g(2)]c[dh]e");
        PI_CHECK_LR(input, resolver, "a[bf(3){0}g(2){1}]c[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abfgcdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "abche<0,0,0,1,0>|"
                                                      "afe<0,1,0>|"
                                                      "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>|"
                                                          "che<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");

        resolver.AllowedLabels.Reset(0);
        resolver.AllowedLabels.Reset(2);
        PI_CHECK_R(input, resolver, "a[bg(2)]cde");
        PI_CHECK_LR(input, resolver, "a[bg(2){1}]cde");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abgcde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "de");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "agde<0,2,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>");
    }

    Y_UNIT_TEST(ResolverChain) {
        TTestInput input("abcde");
        input.Alter(1, 'f', 3, 0);
        input.Alter(1, 'g', 2, 1);
        input.Alter(3, 'h', 1, 2);

        TTestInput::TChainResolver<TTestResolver, TTestResolver> resolver;
        PI_CHECK_R(input, resolver, "a[bf(3)g(2)]c[dh]e");
        PI_CHECK_LR(input, resolver, "a[bf(3){0}g(2){1}]c[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abfgcdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "cdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abcde<0,0,0,0,0>|"
                                                      "abche<0,0,0,1,0>|"
                                                      "afe<0,1,0>|"
                                                      "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "cde<0,0,0>|"
                                                          "che<0,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");

        TDynBitMap allAlts;
        allAlts.Set(0, input.GetMaxAltsCount());
        resolver.FirstResolver.Disabled.assign(input.GetLength(), allAlts);
        resolver.SecondResolver.Disabled.assign(input.GetLength(), allAlts);
        PI_CHECK_R(input, resolver, "-----");
        PI_CHECK_LR(input, resolver, "-----");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "");

        for (size_t pos = 0; pos < input.GetLength(); ++pos) {
            resolver.FirstResolver.Disabled[pos].Reset(0);
        }
        for (size_t pos = 0; pos < input.GetLength(); ++pos) {
            resolver.SecondResolver.Disabled[pos].Reset(1);
        }
        PI_CHECK_R(input, resolver, "-----");
        PI_CHECK_LR(input, resolver, "-----");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "");

        resolver.SecondResolver.Disabled[0].Reset(0);
        resolver.SecondResolver.Disabled[3].Reset(0);
        resolver.SecondResolver.Disabled[4].Reset(0);
        PI_CHECK_R(input, resolver, "a--de");
        PI_CHECK_LR(input, resolver, "a--de");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "a");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "de");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "a<0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>");

        resolver.FirstResolver.Disabled[3].Reset(1);
        resolver.FirstResolver.Disabled[1].Reset(2);
        resolver.SecondResolver.Disabled[1].Reset(2);
        PI_CHECK_R(input, resolver, "ag(2)-[dh]e");
        PI_CHECK_LR(input, resolver, "ag(2){1}-[dh{2}]e");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "agdhe");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 3, resolver, "dhe");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "agde<0,2,0,0>|"
                                                      "aghe<0,2,1,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 3, resolver, "de<0,0>|"
                                                          "he<1,0>");
    }

    Y_UNIT_TEST(Overlap) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'B', 2, 1), 1);
        UNIT_ASSERT_EQUAL(input.Alter(2, 'C', 2, 2), 1);
        PI_CHECK(input, "a[bB(2)][cC(2)]de");
        PI_CHECK_L(input, "a[bB(2){1}][cC(2){2}]de");
        PI_CHECK_TRAVERSE_SYMBOLS(input, "abBcCde");
        PI_CHECK_TRAVERSE_SYMBOLS_O(input, 2, "cCde");
        PI_CHECK_TRAVERSE_BRANCHES(input, "abcde<0,0,0,0,0>|"
                                          "abCe<0,0,1,0>|"
                                          "aBde<0,1,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_O(input, 2, "cde<0,0,0>|"
                                               "Ce<1,0>");
    }

    Y_UNIT_TEST(OverlapTraverseByTrack) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'B', 2, 1), 1);
        UNIT_ASSERT_EQUAL(input.Alter(2, 'C', 2, 2), 1);
        PI_CHECK_TRAVERSE_BRANCH(input, "abcde", 0, 0, 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input, "aBde", 0, 1, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH(input, "abCe", 0, 0, 1, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input, 2, "cde", 0, 0, 0);
        PI_CHECK_TRAVERSE_BRANCH_O(input, 2, "Ce", 1, 0);
    }

    Y_UNIT_TEST(OverlapRegicide) {
        TTestInput input("abcde");
        UNIT_ASSERT_EQUAL(input.Alter(1, 'B', 2, 1), 1);
        UNIT_ASSERT_EQUAL(input.Alter(2, 'C', 2, 2), 1);
        TTestResolver resolver;
        resolver.Disable(2, 0);
        PI_CHECK_R(input, resolver, "a[bB(2)]C(2)de");
        PI_CHECK_LR(input, resolver, "a[bB(2){1}]C(2){2}de");
        PI_CHECK_TRAVERSE_SYMBOLS_R(input, resolver, "abBCde");
        PI_CHECK_TRAVERSE_SYMBOLS_OR(input, 2, resolver, "Ce");
        PI_CHECK_TRAVERSE_BRANCHES_R(input, resolver, "abCe<0,0,1,0>|"
                                                      "aBde<0,1,0,0>");
        PI_CHECK_TRAVERSE_BRANCHES_OR(input, 2, resolver, "Ce<1,0>");
    }
}
