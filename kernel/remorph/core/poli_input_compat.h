#pragma once

#include "poli_input.h"

#include <util/generic/bitmap.h>
#include <util/generic/deque.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>
#include <utility>

// This wrapper is written to describe, document and test dirrefences/relations between old input tree interface and
// new poli-input structure. Do not use this wrapper for production purposes.

namespace NRemorph {

template <typename TSymbol>
class TPoliInputCompat {
public:
    using TRawInput = TPoliInput<TSymbol>;
    using TRawInputPtr = TPoliInputPtr<TSymbol>;
    using TRawResolver = typename TRawInput::TPresetResolver;
    using TAltsMask = typename TRawInput::TAltsMask;

    struct TResolver: public TRawResolver {
        TRawInputPtr RawInput;

        TResolver(TRawInputPtr rawInput)
            : TRawResolver()
            , RawInput(rawInput)
        {
        }

        inline void operator ()(TDynBitMap& alts, size_t pos) const {
            TRawResolver::operator ()(alts, pos, RawInput->GetSymbol(pos, 0), RawInput->GetMarks(pos), RawInput->GetLabels(pos));
        }
    };

private:
    TIntrusivePtr<TRawInput> RawInput;
    TResolver Resolver;
    TAltsMask Accepts;
    bool AcceptAny;
    size_t MaxSymbolLength;

public:
    TPoliInputCompat()
        : RawInput(new TRawInput(TVector<TSymbol>()))
        , Resolver(RawInput)
        , Accepts()
        , AcceptAny(true)
        , MaxSymbolLength(0)
    {
    }

    inline const TRawInput& GetRawInput() const {
        Y_ASSERT(RawInput);
        return *RawInput;
    }

    inline const TResolver& GetResolver() const {
        return Resolver;
    }

    inline bool GetAccept(size_t pos, size_t alt) const {
        Y_ASSERT(pos < Accepts.size());
        return Accepts[pos].Test(alt);
    }

    inline bool GetAcceptAny() const {
        return AcceptAny;
    }

    inline void Clear() {
        RawInput = TRawInputPtr(new TRawInput(TVector<TSymbol>()));
        Resolver.RawInput = RawInput;
        Resolver.AllowedAlts.clear();
        Accepts.clear();
        AcceptAny = true;
        MaxSymbolLength = 0;
    }

    inline void Fill(const TStringBuf::const_iterator& begin, const TStringBuf::const_iterator& end) {
        RawInput = TRawInputPtr(new TRawInput(TVector<TSymbol>(begin, end)));
        Resolver.RawInput = RawInput;
        TDynBitMap onlyMaster;
        onlyMaster.Set(0);
        Resolver.AllowedAlts.assign(RawInput->GetLength(), onlyMaster);
        Accepts.assign(RawInput->GetLength(), onlyMaster);
        AcceptAny = true;
        MaxSymbolLength = 1;
    }

    inline bool CreateBranch(size_t start, size_t end, const TSymbol& symbol, bool replace) {
        Y_ASSERT(RawInput);
        Y_ASSERT(Resolver.AllowedAlts.size() == RawInput->GetLength());
        Y_ASSERT(Accepts.size() == RawInput->GetLength());
        Y_ASSERT(start < end);
        Y_ASSERT(end <= RawInput->GetLength());
        Y_ASSERT(MaxSymbolLength);
        if (!AllowedStart(start) || !AllowedEnd(end)) {
            return false;
        }
        // Original input tree branch replacement logic is somewhat complex in terms of poli-input:
        // Input tree strives to preserve symbols that compose overlapping branches (i.e. branches that have symbols
        // overlapping start or end positions), removing all other symbols from the replacement range.
        // This implementation composes alts keep-mask in two passes
        if (replace) {
            TAltsMask keep(end - start);
            TDynBitMap keepAll;
            // Possible overlaps.
            if (MaxSymbolLength > 1) {
                keepAll.Reserve(RawInput->GetLength());
                TDynBitMap keepFromStart;
                // Marking ends of symbols that overlap start pos.
                for (size_t pos = ::Max(static_cast<size_t>(0), start - MaxSymbolLength + 1); pos < start; ++pos) {
                    Y_FOR_EACH_BIT(alt, Resolver.AllowedAlts[pos]) {
                        size_t step = RawInput->GetSymbolLength(pos, alt);
                        if ((pos + step > start) && (pos + step < end)) {
                            // Keep links to all following symbols.
                            keepAll.Set(pos + step - start);
                        }
                    }
                }
                // Marking following symbols in range.
                for (size_t pos = start + 1; pos < end - 1; ++pos) {
                    if (!keepAll.Test(pos - start)) {
                        continue;
                    }
                    Y_FOR_EACH_BIT(alt, Resolver.AllowedAlts[pos]) {
                        size_t step = RawInput->GetSymbolLength(pos, alt);
                        if (pos + step < end) {
                            // Keep links to all following symbols.
                            keepAll.Set(pos + step - start);
                        }
                    }
                }
                // Marking starts of symbols that overlap end pos.
                for (size_t pos = ::Max(start, end - MaxSymbolLength + 1); pos < end; ++pos) {
                    Y_FOR_EACH_BIT(alt, Resolver.AllowedAlts[pos]) {
                        size_t step = RawInput->GetSymbolLength(pos, alt);
                        if (pos + step > end) {
                            // Keep link from preciding symbol.
                            keep[pos - start].Set(alt);
                        }
                    }
                }
                // Marking preceding symbols in range.
                if (end - start > 1) {
                    for (size_t pos = end - 2;; --pos) {
                        Y_FOR_EACH_BIT(alt, Resolver.AllowedAlts[pos]) {
                            size_t step = RawInput->GetSymbolLength(pos, alt);
                            if ((pos + step < end) && !keep[pos + step - start].Empty()) {
                                keep[pos - start].Set(alt);
                            }
                        }
                        if (pos == start) {
                            break;
                        }
                    }
                }
            }
            // Clear unmarked symbols.
            for (size_t pos = start; pos < end; ++pos) {
                if (!keepAll.Test(pos - start)) {
                    Resolver.AllowedAlts[pos] = keep[pos - start];
                }
            }
        }
        size_t newAlt = RawInput->Alter(start, symbol, end - start);
        MaxSymbolLength = ::Max(MaxSymbolLength, end - start);
        Resolver.AllowedAlts[start].Set(newAlt);
        Accepts[start].Set(RawInput->GetAltsCount(start) - 1);
        return true;
    }

    template <typename TAcceptor>
    inline bool Filter(const TAcceptor& acceptor) {
        Y_ASSERT(RawInput);
        Y_ASSERT(Resolver.AllowedAlts.size() == RawInput->GetLength());
        Y_ASSERT(Accepts.size() == RawInput->GetLength());
        if (!RawInput->GetLength()) {
            AcceptAny = false;
            return AcceptAny;
        }
        TDynBitMap keep;
        keep.Reserve(RawInput->GetLength());
        for (size_t pos = 0; pos < RawInput->GetLength() - 1; ++pos) {
            Y_FOR_EACH_BIT(alt, Resolver.AllowedAlts[pos]) {
                if (keep.Test(pos) || acceptor(RawInput->GetSymbol(pos, alt))) {
                    size_t step = RawInput->GetSymbolLength(pos, alt);
                    if (pos + step < RawInput->GetLength()) {
                        keep.Set(pos + step);
                    }
                }
            }
        }
        for (size_t pos = RawInput->GetLength() - 1;; --pos) {
            Accepts[pos].Clear();
            Y_FOR_EACH_BIT(alt, Resolver.AllowedAlts[pos]) {
                size_t step = RawInput->GetSymbolLength(pos, alt);
                if (((pos + step < Accepts.size()) && !Accepts[pos + step].Empty()) ||
                        acceptor(RawInput->GetSymbol(pos, alt))) {
                    Accepts[pos].Set(alt);
                }
            }
            if (!keep.Test(pos)) {
                Resolver.AllowedAlts[pos] = Accepts[pos];
            }
            if (!pos) {
                break;
            }
        }
        if (Accepts[0].Empty()) {
            Clear();
            AcceptAny = false;
            return AcceptAny;
        }
        AcceptAny = true;
        return AcceptAny;
    }

    template <class TAction>
    void TraverseSymbols(TAction& action) const {
        Y_ASSERT(RawInput);

        // Input tree symbol traversal is reversed in terms of branches.
        // We will save branch traversal tracks, and then parse them from end, handling symbols.

        using TSymbols = THashSet<TSymbol>;

        struct TSubAdapter {
            TAction& Action;
            TSymbols Symbols;
            bool Cut;

            TSubAdapter(TAction& theAction)
                : Action(theAction)
                , Symbols()
                , Cut(false)
            {
            }

            inline void Begin(size_t length) {
                Y_UNUSED(length);
            }

            inline bool operator ()(const TSymbol& symbol) {
                std::pair<typename TSymbols::iterator, bool> inserted = Symbols.insert(symbol);
                if (!inserted.second) {
                    return true;
                }
                Cut = !Action(symbol);
                return !Cut;
            }

            inline void End() {
            }

            inline void Reset() {
                Symbols.clear();
                Cut = false;
            }
        };

        struct TAdapter {
            TRawInput& RawInput;
            TSubAdapter SubAdapter;
            TDeque<TVector<size_t>> Tracks;
            size_t Length;

            TAdapter(TRawInput& rawInput, TAction& theAction)
                : RawInput(rawInput)
                , SubAdapter(theAction)
                , Tracks()
                , Length()
            {
            }

            inline void Begin(size_t length) {
                SubAdapter.Reset();
                Tracks.clear();
                Length = length;
                AddNewTrack();
            }

            inline bool Symbol(const TSymbol& symbol, size_t alt) {
                Y_UNUSED(symbol);
                Y_ASSERT(Tracks);
                Tracks.front().push_back(alt);
                return true;
            }

            inline bool Branch() {
                AddNewTrack();
                return true;
            }

            inline void End() {
                Y_ASSERT(Tracks);
                Y_ASSERT(!Tracks.front());
                for (const auto& track: Tracks) {
                    RawInput.TraverseBranch(SubAdapter, track);
                    if (SubAdapter.Cut) {
                        break;
                    }
                }
            }

            inline void AddNewTrack() {
                Tracks.push_front(TVector<size_t>());
                Tracks.front().reserve(Length);
            }
        };

        TAdapter adapter(*RawInput, action);
        RawInput->TraverseBranches(adapter);
    }

    template <class TAction>
    void TraverseBranches(TAction& action) const {
        struct TAdapter {
            TAction& Action;
            TVector<TSymbol> CurBranch;
            TVector<size_t> CurTrack;

            TAdapter(TAction& theAction)
                : Action(theAction)
                , CurBranch()
                , CurTrack()
            {
            }

            inline void Begin(size_t length) {
                CurBranch.reserve(length);
                CurTrack.reserve(length);
                Clear();
            }

            inline bool Symbol(const TSymbol& symbol, size_t alt) {
                CurBranch.push_back(symbol);
                CurTrack.push_back(alt);
                return true;
            }

            inline bool Branch() {
                bool result = Action(CurBranch, CurTrack);
                Clear();
                return result;
            }

            inline void End() {
            }

            inline void Clear() {
                CurBranch.clear();
                CurTrack.clear();
            }
        };

        TAdapter adapter(action);
        RawInput->TraverseBranches(adapter);
    }

private:
    inline bool AllowedStart(size_t pos) const {
        Y_ASSERT(pos < RawInput->GetLength());
        Y_ASSERT(MaxSymbolLength);
        if (!pos) {
            return true;
        }
        bool allowed = false;
        for (size_t p = pos - 1;; --p) {
            Y_FOR_EACH_BIT(alt, Resolver.AllowedAlts[p]) {
                if (p + RawInput->GetSymbolLength(p, alt) == pos) {
                    allowed = true;
                    break;
                }
            }
            if (allowed || (p == pos - MaxSymbolLength) || !p) {
                break;
            }
        }
        return allowed;
    }

    inline bool AllowedEnd(size_t pos) const {
        Y_ASSERT(pos && (pos <= RawInput->GetLength()));
        if (pos == RawInput->GetLength()) {
            return true;
        }
        return !Resolver.AllowedAlts[pos].Empty();
    }
};

} // NRemorph
