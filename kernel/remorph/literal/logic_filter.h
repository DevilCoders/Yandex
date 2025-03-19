#pragma once

#include "logic_expr.h"

#include <kernel/remorph/core/core.h>

#include <util/generic/algorithm.h>
#include <util/generic/bitmap.h>

namespace NLiteral {

namespace NPrivate {

// For using with TInput::Filter
template <class TSymbolPtr>
struct TLogicInputFilter {
    const TLInstrVector& LogicExp;
    TDynBitMap& Ctx;

    TLogicInputFilter(const TLInstrVector& le, TDynBitMap& ctx)
        : LogicExp(le)
        , Ctx(ctx)
    {
    }

    inline bool operator() (const TSymbolPtr& s) const {
        Ctx.Clear();
        return Exec(LogicExp, *s, Ctx);
    }
};

// For using with TInput::TraverseSymbols
template <class TSymbolPtr>
struct TLogicInputChecker {
    const TLInstrVector& LogicExp;
    TDynBitMap Ctx;
    bool Res;

    explicit TLogicInputChecker(const TLInstrVector& le)
        : LogicExp(le)
        , Ctx()
        , Res(false)
    {
    }

    inline bool operator() (const TSymbolPtr& s) {
        Ctx.Clear();
        if (Exec(LogicExp, *s, Ctx))
            Res = true;
        // Stop traversing when have found a symbol
        return !Res;
    }
};

} // NPrivate

// Remains only branches, which have at least one input symbol, which conforms the specified filter,
// and removes all other branches. Returns true if input has at least one sub-branch after removal.
template <class TSymbolPtr>
inline bool FilterInputByLogicExp(NRemorph::TInput<TSymbolPtr>& input, const TLInstrVector& logicExp) {
    if (logicExp.empty()) {
        return true;
    } else {
        TDynBitMap ctx;
        return input.Filter(NPrivate::TLogicInputFilter<TSymbolPtr>(logicExp, ctx));
    }
}

// Return offset to the last symbol, which conforms the specified filter, or zero if no such symbol
template <class TSymbolPtr>
inline size_t GetLastSymbolByLogicExp(const TVector<TSymbolPtr>& vec, const TLInstrVector& logicExp) {
    if (logicExp.empty()) {
        return vec.size();
    } else {
        TDynBitMap ctx;
        const size_t off = std::distance(vec.begin(),
            FindIf(vec.rbegin(), vec.rend(), NPrivate::TLogicInputFilter<TSymbolPtr>(logicExp, ctx)).base());
        return off;
    }
}

// Returns true if input has at least one input symbol, which conforms the specified filter
template <class TSymbolPtr>
inline bool InputHasLogicExp(const NRemorph::TInput<TSymbolPtr>& input, const TLInstrVector& logicExp) {
    if (logicExp.empty()) {
        return true;
    } else {
        NPrivate::TLogicInputChecker<TSymbolPtr> logicInputChecker(logicExp);
        input.TraverseSymbols(logicInputChecker);
        return logicInputChecker.Res;
    }
}

} // NLiteral
