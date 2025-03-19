#pragma once

#include <kernel/remorph/input/properties.h>

#include <library/cpp/solve_ambig/occ_traits.h>

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <utility>

namespace NReMorph {

// Emulates the syntax tree
struct TSyntaxChunk {
    std::pair<size_t, size_t> Pos; // Chunk positions [first, second)
    float Weight;
    TString Name; // Type of syntax phrase
    TVector<TSyntaxChunk> Children;
    int Depth;
    i64 Head;

    TSyntaxChunk();
    TSyntaxChunk(size_t start, size_t end);
    TSyntaxChunk(const std::pair<size_t, size_t>& pos, float weight, const TString& name);

    inline bool operator <(const TSyntaxChunk& rh) const {
        return Pos < rh.Pos;
    }

    size_t Length() const {
        return Pos.second - Pos.first;
    }

    static inline bool Include(const std::pair<size_t, size_t>& range, size_t pos) {
        return range.first <= pos && pos < range.second;
    }

    static inline bool StrongInclude(const std::pair<size_t, size_t>& range, size_t pos) {
        return range.first < pos && pos < range.second;
    }

    static inline bool Include(const std::pair<size_t, size_t>& range, const std::pair<size_t, size_t>& subRange) {
        return range.first <= subRange.first && subRange.second <= range.second;
    }

    static inline bool Conflict(const std::pair<size_t, size_t>& range, const std::pair<size_t, size_t>& otherRange) {
        return range.first != otherRange.first
            && range.second != otherRange.second
            && StrongInclude(range, otherRange.first) != StrongInclude(range, otherRange.second);
    }

    inline bool Include(size_t p) const {
        return Include(Pos, p);
    }

    inline bool StrongInclude(size_t p) const {
        return StrongInclude(Pos, p);
    }

    // This chunk fully includes positions of the specified chunk
    inline bool Include(const std::pair<size_t, size_t>& range) const {
        return Include(Pos, range);
    }

    // This chunk have conflicting positions with the specified one
    inline bool Conflict(const std::pair<size_t, size_t>& range) const {
        return Conflict(Pos, range);
    }

    // Calc and return current chunk depth. Bottom chunks have level 1
    int GetDepth();

    // Add child chunk in proper level. The position of added chunk must belong to the position of the current one
    bool AddChild(const TSyntaxChunk& newChunk);

    // Add new chunk in hierarchical order. If new chunk covers some existing chunks
    // then they are replaced by this new one and are moved to its children.
    // If new chunk is covered by some existing one then it is added to its children.
    // Otherwise it is included into the list as the sibling.
    static bool AddChunk(TVector<TSyntaxChunk>& chunks, const TSyntaxChunk& newChunk);

    void SetHead(const std::pair<size_t, size_t>& headRange);
    bool SetHead(const std::pair<size_t, size_t>& headRange, const std::pair<size_t, size_t>& parentRange);

    static void SetHead(TVector<TSyntaxChunk>& chunks, const std::pair<size_t, size_t>& headRange);
    static bool SetHead(TVector<TSyntaxChunk>& chunks, const std::pair<size_t, size_t>& headRange, const std::pair<size_t, size_t>& parentRange);

    void Swap(TSyntaxChunk& c);

    template <class TSymbolPtr>
    TUtf16String GetText(const TVector<TSymbolPtr>& symbols) {
        Y_ASSERT(Pos.first < symbols.size());
        Y_ASSERT(Pos.second <= symbols.size());
        TUtf16String text;
        typename TVector<TSymbolPtr>::const_iterator symbol = symbols.begin() + Pos.first;
        typename TVector<TSymbolPtr>::const_iterator end = symbols.begin() + Pos.second;
        bool first = true;
        for (; symbol != end; ++symbol) {
            if (!first and (*symbol)->GetProperties().Test(NSymbol::PROP_SPACE_BEFORE)) {
                text.append(' ');
            }
            text.append((*symbol)->GetText());
            first = false;
        }
        return text;
    }

    template <class TSymbolPtr>
    TString ToString(const TVector<TSymbolPtr>& symbols) const {
        Y_ASSERT(Pos.first < symbols.size());
        Y_ASSERT(Pos.second <= symbols.size());
        return TString().append('[').append(Name).append(ToString(Pos.first, Pos.second, Head, Children, symbols)).append(']');
    }

    template <class TSymbolPtr>
    static void AppendToString(TString& res, size_t beg, size_t end, i64 head, const TVector<TSymbolPtr>& symbols) {
        Y_ASSERT(beg < symbols.size());
        Y_ASSERT(end <= symbols.size());
        for (size_t i = beg; i < end; ++i) {
            res.append(' ');
            if (i == static_cast<size_t>(head))
                res.append('+');
            res.append(WideToUTF8(symbols[i]->GetText()));
        }
    }

    template <class TSymbolPtr>
    static TString ToString(size_t beg, size_t end, i64 head, const TVector<TSyntaxChunk>& chunks, const TVector<TSymbolPtr>& symbols) {
        Y_ASSERT(beg < symbols.size());
        Y_ASSERT(end <= symbols.size());
        TString res;
        size_t last = beg;
        for (TVector<TSyntaxChunk>::const_iterator iChild = chunks.begin(); iChild != chunks.end(); ++iChild) {
            if (iChild->Pos.first > last) {
                AppendToString(res, last, iChild->Pos.first, head, symbols);
            }
            res.append(' ');
            if (Include(iChild->Pos, head)) {
                res.append('+');
            }
            res.append(iChild->ToString(symbols));
            last = iChild->Pos.second;
        }
        if (last < end) {
            AppendToString(res, last, end, head, symbols);
        }
        return res;
    }

    template <class TSymbolPtr>
    static TString ToString(const TVector<TSyntaxChunk>& chunks, const TVector<TSymbolPtr>& symbols) {
        return ToString(0, symbols.size(), -1, chunks, symbols);
    }

    // Calculate total coverage
    static size_t Coverage(const TVector<TSyntaxChunk>& chunks);

    static double Quality(size_t wordCount, TVector<TSyntaxChunk>& chunks);

    template <class TOp>
    bool Iter(TOp& op) {
        return op(*this) && Iter(Children, op);
    }

    template <class TOp>
    bool Iter(TOp& op) const {
        return op(*this) && Iter(Children, op);
    }

    template <class TOp>
    static bool Iter(TVector<TSyntaxChunk>& chunks, TOp& op) {
        for (size_t i = 0; i < chunks.size(); ++i) {
            if (!chunks[i].Iter(op))
                return false;
        }
        return true;
    }

    template <class TOp>
    static bool Iter(const TVector<TSyntaxChunk>& chunks, TOp& op) {
        for (size_t i = 0; i < chunks.size(); ++i) {
            if (!chunks[i].Iter(op))
                return false;
        }
        return true;
    }
};

typedef TVector<TSyntaxChunk> TSyntaxChunks;

} // NReMorph

namespace NSolveAmbig {

template <>
struct TOccurrenceTraits<NReMorph::TSyntaxChunk> {
    inline static TStringBuf GetId(TTypeTraits<NReMorph::TSyntaxChunk>::TFuncParam c) {
        return c.Name;
    }
    inline static size_t GetCoverage(TTypeTraits<NReMorph::TSyntaxChunk>::TFuncParam c) {
        return c.Length();
    }
    inline static size_t GetStart(TTypeTraits<NReMorph::TSyntaxChunk>::TFuncParam c) {
        return c.Pos.first;
    }
    inline static size_t GetStop(TTypeTraits<NReMorph::TSyntaxChunk>::TFuncParam c) {
        return c.Pos.second;
    }
    inline static double GetWeight(TTypeTraits<NReMorph::TSyntaxChunk>::TFuncParam c) {
        return double(c.Weight);
    }
};

} // NSolveAmbig
