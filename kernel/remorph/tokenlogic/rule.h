#pragma once

#include "token_expr.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/literal/agreement.h>
#include <kernel/remorph/literal/literal_table.h>

#include <util/stream/output.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/bitmap.h>
#include <util/generic/ylimits.h>
#include <utility>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NTokenLogic {

namespace NPrivate {

class TMatchContext {
public:
    std::pair<size_t, size_t> Range;
    TVector<std::pair<TString, size_t>> NamedMatches;
    TVector<TDynBitMap> Contexts;
    TVector<i32> LiteralCounts;
    NSorted::TSimpleMap<size_t, TDynBitMap> LiteralMatches;
    i32 OtherCount;

    TMatchContext(size_t symbolCount, size_t literalCount)
        : Range(Max<size_t>(), Max<size_t>())
        , Contexts(symbolCount)
        , OtherCount(-1)
    {
        LiteralCounts.resize(literalCount, -1);
    }

protected:
    void Reset() {
        Range.first = Range.second = Max<size_t>();
        NamedMatches.clear();
        // Don't clear vector. Reuse already allocated buffers
        for (TVector<TDynBitMap>::iterator iCtx = Contexts.begin(); iCtx != Contexts.end(); ++iCtx) {
            iCtx->Clear();
        }
        OtherCount = -1;
        ::Fill(LiteralCounts.begin(), LiteralCounts.end(), -1);
        LiteralMatches.clear();
    }

    void PutToRange(size_t pos) {
        if (Range.first == Max<size_t>()) {
            Range.first =  pos;
            Range.second = pos + 1;
        } else {
            Range.first =  Min(Range.first, pos);
            Range.second = Max(Range.second, pos + 1);
        }
    }

    void PutToNamedMatches(const TTokenExpCmp& cmpInstr, size_t pos) {
        const NSorted::TSortedVector<TString>& labels = cmpInstr.Labels;
        for (size_t l = 0; l < labels.size(); ++l) {
            NamedMatches.push_back(std::make_pair(labels[l], pos));
        }
    }
};

template <class TSymbolPtr>
class TRuleContext: public TMatchContext {
public:
    const TVector<TSymbolPtr>& Symbols;
    const NLiteral::TLiteralTable& LT;
    TDynBitMap MatchedSymbols;
    NLiteral::TAgreementContext AgreeCtx;
    TVector<const TTokenExpCmp*> OrdinalExps;
    TVector<const TTokenExpCmp*> LabeledAnyExps;
    bool HasAny;

    TRuleContext(const TVector<TSymbolPtr>& symbols, const NLiteral::TLiteralTable& lt)
        : TMatchContext(symbols.size(), lt.Size())
        , Symbols(symbols)
        , LT(lt)
        , HasAny(false)
    {
        MatchedSymbols.Reserve(Symbols.size());
    }

    void Reset(const TTokenExpVector& exp, bool needFullScan) {
        TMatchContext::Reset();
        ClearMatchCache(Symbols);
        AgreeCtx.clear();
        MatchedSymbols.Clear();
        OrdinalExps.clear();
        LabeledAnyExps.clear();
        HasAny = false;

        for (TTokenExpVector::const_iterator iExp = exp.begin(); iExp != exp.end(); ++iExp) {
            if (TTokenExpBase::Cmp != iExp->Get()->Type)
                continue;
            const TTokenExpCmp* cmpExp = static_cast<const TTokenExpCmp*>(iExp->Get());
            Y_ASSERT(cmpExp->Lit.IsOrdinal() || cmpExp->Lit.IsAny());
            if (cmpExp->Lit.IsOrdinal()) {
                OrdinalExps.push_back(cmpExp);
            } else if (cmpExp->Lit.IsAny()) {
                HasAny = true;
                if (!cmpExp->Labels.empty()) {
                    LabeledAnyExps.push_back(cmpExp);
                }
            }
        }
        if (needFullScan) {
            FullScan();
        }
    }

    ui32 GetCount(const TTokenExpCmp& cmpInstr) {
        const NRemorph::TLiteral lit = cmpInstr.Lit;
        Y_ASSERT(!lit.IsOrdinal() || lit.GetId() < LiteralCounts.size());
        if (lit.IsOrdinal()) {
            return CalcOrdinalCount(lit);
        }
        Y_ASSERT(lit.IsAny());
        CalcAllCounts();
        return static_cast<ui32>(OtherCount);
    }

    void Finalize() {
        // Calc all literals, which are not calculated yet.
        // This step is required to properly calculate OtherCount and include all matched named literals
        CalcAllCounts();

        // For all named literals remember the named matches
        for (TVector<const TTokenExpCmp*>::const_iterator iExp = OrdinalExps.begin(); iExp != OrdinalExps.end(); ++iExp) {
            const TTokenExpCmp& cmpExp = **iExp;
            if (!cmpExp.Labels.empty()) {
                NSorted::TSimpleMap<size_t, TDynBitMap>::const_iterator iMatch = LiteralMatches.find(cmpExp.Lit.GetId());
                if (iMatch != LiteralMatches.end()) {
                    Y_FOR_EACH_BIT(i, iMatch->second) { // Mask of matched symbols
                        PutToNamedMatches(cmpExp, i);
                    }
                }
            }
        }

        if (HasAny && OtherCount > 0) {
            if (!LabeledAnyExps.empty()) {
                TDynBitMap otherSymbols = ~MatchedSymbols; // Mask of other symbols
                otherSymbols.Reset(Symbols.size(), otherSymbols.Size()); // Clear upper bits
                for (TVector<const TTokenExpCmp*>::const_iterator iExp = LabeledAnyExps.begin(); iExp != LabeledAnyExps.end(); ++iExp) {
                    const TTokenExpCmp& anyExp = **iExp;
                    Y_FOR_EACH_BIT(i, otherSymbols) {
                        PutToNamedMatches(anyExp, i);
                    }
                }
            }
            // If we have "." in rule then include all symbols into the matched range
            Range.first =  0;
            Range.second = Symbols.size();
        } else {
            // Calc the matched range using mask of matched symbols
            Y_FOR_EACH_BIT(i, MatchedSymbols) {
                PutToRange(i);
            }
        }
    }

    inline bool Empty() const {
        return MatchedSymbols.Empty() && (!HasAny || 0 == OtherCount);
    }

protected:
    // Calculate and cache the count of matched symbols for the specified literal
    ui32 CalcOrdinalCount(const NRemorph::TLiteral lit) {
        Y_ASSERT(lit.IsOrdinal());
        Y_ASSERT(lit.GetId() < LiteralCounts.size());
        i32& cnt = LiteralCounts[lit.GetId()];
        if (cnt < 0) {
            cnt = 0;
            TDynBitMap litToSymbol;
            for (size_t i = 0; i < Symbols.size(); ++i) {
                if (LT.IsEqual(lit, Symbols[i], Contexts[i])
                    && LT.CheckAgreements(lit, Symbols[i].Get(), Contexts[i], AgreeCtx)) {
                    ++cnt;
                    litToSymbol.Set(i);
                }
            }
            if (cnt > 0) {
                MatchedSymbols |= litToSymbol;
                DoSwap(LiteralMatches[lit.GetId()], litToSymbol);
            }
        }
        return static_cast<ui32>(cnt);
    }

    void CalcAllCounts() {
        if (-1 != OtherCount)
            return;

        for (TVector<const TTokenExpCmp*>::const_iterator iExp = OrdinalExps.begin(); iExp != OrdinalExps.end(); ++iExp) {
            CalcOrdinalCount((*iExp)->Lit);
        }

        Y_ASSERT(MatchedSymbols.Count() <= Symbols.size());
        OtherCount = Symbols.size() - MatchedSymbols.Count();
    }

    void FullScan() {
        for (size_t i = 0; i < Symbols.size(); ++i) {
            for (TVector<const TTokenExpCmp*>::const_iterator iExp = OrdinalExps.begin(); iExp != OrdinalExps.end(); ++iExp) {
                const NRemorph::TLiteral lit = (*iExp)->Lit;
                i32& cnt = LiteralCounts[lit.GetId()];
                if (cnt < 0) {
                    cnt = 0;
                }
                if (LT.IsEqual(lit, Symbols[i], Contexts[i])
                    && LT.CheckAgreements(lit, Symbols[i].Get(), Contexts[i], AgreeCtx)) {
                    ++cnt;
                    MatchedSymbols.Set(i);
                    LiteralMatches[lit.GetId()].Set(i);
                }
            }
        }

        Y_ASSERT(MatchedSymbols.Count() <= Symbols.size());
        OtherCount = Symbols.size() - MatchedSymbols.Count();
    }
};

struct TRule {
    TString Name;
    TTokenExpVector Expression;
    double Weight;
    // Heavy rule requires a full scan before executing the expression.
    // Currently, a rule is heavy if it has distance agreement, because such agreement requires
    // sequential symbol processing order.
    bool HeavyFlag;

    TRule()
        : Weight(1.0)
        , HeavyFlag(false)
    {
    }

    TRule(const TString& name, const TTokenExpVector& exp, double weight, bool heavy)
        : Name(name)
        , Expression(exp)
        , Weight(weight)
        , HeavyFlag(heavy)
    {
    }

    template <class TSymbolPtr>
    bool Matches(TRuleContext<TSymbolPtr>& ctx) const {
        ctx.Reset(Expression, HeavyFlag);
        if (EvalTokenExp(Expression, ctx) && !ctx.Empty()) { // Rule must match at least one token
            ctx.Finalize();
            return true;
        }
        return false;
    }

    TString ToString(const NLiteral::TLiteralTable& lt) const {
        return TString(Name).append("(").append(::ToString(Weight)).append("): ").append(NTokenLogic::ToString(lt, Expression));
    }

    void Save(IOutputStream* out) const {
        ::Save(out, Name);
        ::Save(out, Expression);
        ::Save(out, Weight);
        ::Save(out, static_cast<ui8>(HeavyFlag));
    }

    void Load(IInputStream* in) {
        ::Load(in, Name);
        ::Load(in, Expression);
        ::Load(in, Weight);
        ui8 heavy;
        ::Load(in, heavy);
        HeavyFlag = heavy != 0;
    }
};

} // NPrivate

} // NTokenLogic
