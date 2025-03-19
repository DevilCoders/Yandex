#pragma once

#include <util/generic/bitmap.h>
#include <utility>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

namespace NRemorph {

// Multivariant symbol input
template <typename TSymbol>
class TPoliInput: public TAtomicRefCount<TPoliInput<TSymbol>> {
public:
    // Alternate symbol mark
    struct TMark {
        const TSymbol Symbol; // Alternate symbol
        const size_t Length; // Length in terms of master branch
        const size_t Label; // Label
    };

    // Alts configuration
    using TAltsMask = TVector<TDynBitMap>;

    // All-inclusive resolver
    struct TGreedyResolver {
        Y_FORCE_INLINE void operator ()(TDynBitMap& alts, size_t pos, const TSymbol& masterSymbol,
                                       const TVector<TMark>& marks, const TDynBitMap& labels) const {
            Y_UNUSED(alts);
            Y_UNUSED(pos);
            Y_UNUSED(masterSymbol);
            Y_UNUSED(marks);
            Y_UNUSED(labels);
        }
    };

    struct TPresetResolver {
        TAltsMask AllowedAlts; // Allowed transitions

        Y_FORCE_INLINE void operator ()(TDynBitMap& alts, size_t pos, const TSymbol& masterSymbol,
                                       const TVector<TMark>& marks, const TDynBitMap& labels) const {
            Y_UNUSED(masterSymbol);
            Y_UNUSED(marks);
            Y_UNUSED(labels);
            if (pos >= AllowedAlts.size()) {
                alts.Clear();
                return;
            }
            alts &= AllowedAlts[pos];
        }
    };

    // Symbol-filtering resolver
    template <typename TSymbolAcceptor>
    struct TSymbolResolver {
        TSymbolAcceptor Acceptor; // Symbol acceptor

        TSymbolResolver()
            : Acceptor()
        {
        }

        TSymbolResolver(const TSymbolAcceptor& acceptor)
            : Acceptor(TSymbolAcceptor(acceptor))
        {
        }

        TSymbolResolver(TSymbolAcceptor&& acceptor)
            : Acceptor(::std::move(acceptor))
        {
        }

        Y_FORCE_INLINE void operator ()(TDynBitMap& alts, size_t pos, const TSymbol& masterSymbol,
                                       const TVector<TMark>& marks, const TDynBitMap& labels) const {
            Y_UNUSED(pos);
            Y_UNUSED(labels);
            if (!Acceptor(masterSymbol)) {
                alts.Reset(0);
            }
            for (size_t m = 0; m < marks.size(); ++m) {
                if (!Acceptor(marks[m].Symbol)) {
                    alts.Reset(m + 1);
                }
            }
        }
    };

    // Label-filtering resolver
    struct TLabelResolver {
        TDynBitMap AllowedLabels; // Allowed labels

        Y_FORCE_INLINE void operator ()(TDynBitMap& alts, size_t pos, const TSymbol& masterSymbol,
                                       const TVector<TMark>& marks, const TDynBitMap& labels) const {
            Y_UNUSED(pos);
            Y_UNUSED(masterSymbol);
            Y_UNUSED(labels);
            for (size_t m = 0; m < marks.size(); ++m) {
                if (!AllowedLabels.Test(marks[m].Label)) {
                    alts.Reset(m + 1);
                }
            }
        }
    };

    // Chaining resolver
    template <typename TFirstResolver, typename TSecondResolver>
    struct TChainResolver {
        TFirstResolver FirstResolver;
        TSecondResolver SecondResolver;

        TChainResolver()
            : FirstResolver()
            , SecondResolver()
        {
        }

        TChainResolver(const TFirstResolver& firstResolver, const TSecondResolver& secondResolver)
            : FirstResolver(firstResolver)
            , SecondResolver(secondResolver)
        {
        }

        Y_FORCE_INLINE void operator ()(TDynBitMap& alts, size_t pos, const TSymbol& masterSymbol,
                                       const TVector<TMark>& marks, const TDynBitMap& labels) const {
            FirstResolver(alts, pos, masterSymbol, marks, labels);
            SecondResolver(alts, pos, masterSymbol, marks, labels);
        }
    };

private:
    const TVector<TSymbol> Master; // Master branch
    TVector<TVector<TMark>> Marks; // Alternate symbol marks
    TVector<TDynBitMap> Labels; // Mark labels cache
    size_t ExtraSymbols; // Extra symbols count
    size_t MaxAltsCount; // Maximum transitions count
    TDynBitMap AllLabels; // All labels cache

public:
    TPoliInput(const TVector<TSymbol>& input)
        : TPoliInput(TVector<TSymbol>(input))
    {
    }

    TPoliInput(TVector<TSymbol>&& input)
        : Master(::std::move(input))
        , Marks(Master.size())
        , Labels(Master.size())
        , ExtraSymbols(0)
        , MaxAltsCount(Master.empty() ? 0 : 1)
        , AllLabels()
    {
    }

    Y_FORCE_INLINE const TVector<TSymbol>& GetMaster() const {
        return Master;
    }

    Y_FORCE_INLINE size_t GetLength() const {
        Y_ASSERT(Marks.size() == Master.size());
        Y_ASSERT(Labels.size() == Master.size());

        return Master.size();
    }

    Y_FORCE_INLINE size_t GetExtraSymbols() const {
        return ExtraSymbols;
    }

    Y_FORCE_INLINE size_t GetMaxAltsCount() const {
        return MaxAltsCount;
    }

    Y_FORCE_INLINE const TDynBitMap& GetAllLabels() const {
        return AllLabels;
    }

    Y_FORCE_INLINE size_t GetAltsCount(size_t pos) const {
        Y_ASSERT(pos < GetLength());

        return Marks[pos].size() + 1;
    }

    Y_FORCE_INLINE const TSymbol& GetSymbol(size_t pos, size_t alt) const {
        Y_ASSERT(pos < GetLength());
        Y_ASSERT(alt <= Marks[pos].size());

        if (!alt) {
            return Master[pos];
        }

        return alt ? Marks[pos][alt - 1].Symbol : Master[pos];

    }

    Y_FORCE_INLINE size_t GetSymbolLength(size_t pos, size_t alt) const {
        Y_ASSERT(pos < GetLength());
        Y_ASSERT(alt <= Marks[pos].size());

        if (!alt) {
            return 1;
        }

        Y_ASSERT(Marks[pos][alt - 1].Length);

        return Marks[pos][alt - 1].Length;
    }

    Y_FORCE_INLINE size_t GetLabel(size_t pos, size_t alt) const {
        Y_ASSERT(pos < GetLength());
        Y_ASSERT(alt <= Marks[pos].size());
        Y_ASSERT(alt);

        return Marks[pos][alt - 1].Label;
    }

    Y_FORCE_INLINE const TVector<TMark>& GetMarks(size_t pos) const {
        Y_ASSERT(pos < GetLength());

        return Marks[pos];
    }

    Y_FORCE_INLINE const TDynBitMap& GetLabels(size_t pos) const {
        Y_ASSERT(pos < GetLength());

        return Labels[pos];
    }

    template <typename THandler, typename TResolver = TGreedyResolver>
    Y_FORCE_INLINE void TraverseRaw(THandler& handler, const TResolver& resolver = GetGreedyResolver()) const {
        if (!GetLength()) {
            return;
        }

        handler.Begin(GetLength(), GetExtraSymbols());

        size_t label;
        TDynBitMap alts;
        for (size_t pos = 0; pos < GetLength(); ++pos) {
            handler.Position(pos);
            alts.Clear().Set(0, Marks[pos].size() + 1);
            resolver(alts, pos, Master[pos], Marks[pos], Labels[pos]);
            Y_FOR_EACH_BIT(alt, alts) {
                Y_ASSERT(alt < GetAltsCount(pos));
                if (alt) {
                    label = GetLabel(pos, alt);
                }
                handler.Symbol(GetSymbol(pos, alt), GetSymbolLength(pos, alt), alt ? &label : nullptr);
            }
        }

        handler.End();
    }

    template <typename THandler, typename TResolver = TGreedyResolver>
    Y_FORCE_INLINE void TraverseSymbols(THandler& handler, size_t startPos = 0,
                                       const TResolver& resolver = GetGreedyResolver()) const {
        if (startPos >= GetLength()) {
            return;
        }

        handler.Begin(GetLength() - startPos + GetExtraSymbols());

        TDynBitMap alts;
        TDynBitMap next;
        next.Reserve(GetLength() - startPos);
        size_t step = 0;
        bool stop = false;
        for (size_t pos = startPos; pos < GetLength(); pos += step) {
            Y_ASSERT(pos >= startPos);
            alts.Clear().Set(0, Marks[pos].size() + 1);
            resolver(alts, pos, Master[pos], Marks[pos], Labels[pos]);
            Y_FOR_EACH_BIT(alt, alts) {
                Y_ASSERT(alt < GetAltsCount(pos));
                if (!handler(GetSymbol(pos, alt))) {
                    stop = true;
                    break;
                }
                next.Set(GetSymbolLength(pos, alt) - 1);
            }
            if (stop) {
                break;
            }
            step = next.FirstNonZeroBit() + 1;
            next >>= step;
        }

        handler.End();
    }

    template <typename THandler, typename TResolver = TGreedyResolver>
    Y_FORCE_INLINE void TraverseBranches(THandler& handler, size_t startPos = 0,
                                        const TResolver& resolver = GetGreedyResolver()) const {
        struct TElem {
            size_t Alt;
            size_t StepTo;
            TDynBitMap AltsCache;
            bool AltsCached;
        };

        if (startPos >= GetLength()) {
            return;
        }

        handler.Begin(GetLength() - startPos);

        TVector<TElem> data(GetLength() - startPos);
        bool branchDone = false;
        size_t length = 1;
        for (size_t pos = startPos;;) {
            Y_ASSERT(pos >= startPos);
            Y_ASSERT(pos < GetLength());
            Y_ASSERT(length);

            // Resolve possible transitions (cached)
            if (!data[pos - startPos].AltsCached) {
                data[pos - startPos].AltsCache.Set(0, Marks[pos].size() + 1);
                resolver(data[pos - startPos].AltsCache, pos, Master[pos], Marks[pos], Labels[pos]);
                data[pos - startPos].AltsCached = true;
            }

            // Get next transition
            if (!data[pos - startPos].StepTo) {
                data[pos - startPos].Alt = data[pos - startPos].AltsCache.FirstNonZeroBit();
                data[pos - startPos].StepTo = length;
            } else {
                Y_ASSERT(data[pos - startPos].Alt < data[pos - startPos].AltsCache.Size());
                data[pos - startPos].Alt = data[pos - startPos].AltsCache.NextNonZeroBit(data[pos - startPos].Alt);
            }

            // Check for dead node (no transitions)
            if (data[pos - startPos].AltsCache.Empty() && (pos != startPos)) {
                branchDone = true;
                if (!handler.Branch()) {
                    break;
                }
            }

            // Check for transitions end
            if (data[pos - startPos].Alt >= GetAltsCount(pos)) {
                // Check for traversal end
                if (pos == startPos) {
                    break;
                }
                pos -= data[pos - startPos].StepTo;
                continue;
            }

            // Start branch if needed
            if (branchDone) {
                // Retraverse prefix
                branchDone = false;
                for (size_t hpos = startPos; hpos < pos; hpos += GetSymbolLength(hpos, data[hpos - startPos].Alt)) {
                    Y_ASSERT(hpos >= startPos);
                    if (!handler.Symbol(GetSymbol(hpos, data[hpos - startPos].Alt), data[hpos - startPos].Alt)) {
                        branchDone = true;
                        break;
                    }
                }
                if (branchDone) {
                    if (!handler.Branch()) {
                        break;
                    }
                    continue;
                }
            }

            // Get element and process symbol
            // Check for handler stop or last input symbol
            length = GetSymbolLength(pos, data[pos - startPos].Alt);
            if (!handler.Symbol(GetSymbol(pos, data[pos - startPos].Alt), data[pos - startPos].Alt) ||
                (pos + length >= GetLength())) {
                branchDone = true;
                if (!handler.Branch()) {
                    break;
                }
                continue;
            }

            pos += length;
            data[pos - startPos].StepTo = 0;
        }

        handler.End();
    }

    template <typename THandler>
    Y_FORCE_INLINE void TraverseBranch(THandler& handler, TVector<size_t> track, size_t startPos = 0) const {
        if (startPos >= GetLength()) {
            return;
        }

        Y_ASSERT(track.size() <= GetLength() - startPos);

        handler.Begin(GetLength() - startPos);

        size_t pos = startPos;
        for (auto alt: track) {
            Y_ASSERT(pos >= startPos);
            Y_ASSERT(pos < GetLength());
            Y_ASSERT(alt < GetAltsCount(pos));

            if (!handler(GetSymbol(pos, alt))) {
                break;
            }
            pos += GetSymbolLength(pos, alt);
        }

        handler.End();
    }

    // TODO: emplace_back
    Y_FORCE_INLINE size_t Alter(size_t pos, const TSymbol& symbol, size_t length = 1, size_t label = 0) {
        Y_ASSERT(length != 0);
        Y_ASSERT(pos + length <= GetLength());

        Marks[pos].push_back(TMark{symbol, length, label});
        Labels[pos].Set(label);
        ++ExtraSymbols;
        MaxAltsCount = ::Max(MaxAltsCount, GetAltsCount(pos));
        AllLabels.Set(label);
        return GetAltsCount(pos) - 1;
    }

    Y_FORCE_INLINE static const TGreedyResolver& GetGreedyResolver() {
        return ::Default<TGreedyResolver>();
    }
};

template <typename TSymbol>
using TPoliInputPtr = TIntrusivePtr<TPoliInput<TSymbol>>;

} // NRemorph
