#pragma once

#include <kernel/remorph/proc_base/result_base.h>

#include <library/cpp/solve_ambig/occ_traits.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/stream/output.h>
#include <util/stream/trace.h>
#include <utility>

namespace NTokenLogic {

namespace NPrivate {
    class TMatchContext;
}

typedef NSorted::TSimpleMap<TString, size_t> TNamedTokens;

class TTokenLogicResult: public NMatcher::TResultBase {
public:
    // Positions of the named tokens relative to the matched sequence.
    // Use GetMatchedSymbol() method to extract corresponding symbol
    TNamedTokens NamedTokens;

private:
    void FillResult(const NPrivate::TMatchContext& ctx);

public:
    TTokenLogicResult(const TString& rule, double priority, const std::pair<size_t, size_t>& origRange,
        const NPrivate::TMatchContext& ctx, const TVector<size_t>& track);
    TTokenLogicResult(const TString& rule, double priority, const std::pair<size_t, size_t>& origRange,
        const NPrivate::TMatchContext& ctx);

    void GetNamedRanges(NRemorph::TNamedSubmatches& ranges) const override;

    inline double GetWeight() const {
        return NMatcher::TResultBase::GetWeight() * (NamedTokens.size() > 0 ? NamedTokens.size() : 1);
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    TString ToDebugString(int level, const TInputSource& inputSource) const {
        typedef typename TInputSource::value_type TSymbol;
        TString res;
        TStringOutput out(res);
        out << "rule=" << RuleName << ", pr=" << Priority << ", src=[" << WholeExpr.first << ',' << WholeExpr.second << ')';
        if (int(TRACE_DETAIL) <= level) {
            out << ", named-tokens: " << NamedTokens;
        }
        if (int(TRACE_VERBOSE) <= level) {
            TVector<TSymbol> symbols;
            TVector<TDynBitMap> contexts;
            ExtractMatched(inputSource, symbols, contexts);
            out << ", seq:";
            for (size_t i = 0; i < symbols.size(); ++i) {
                out << ' ' << symbols[i]->ToDebugString(contexts[i]);
            }
        }
        return res;
    }

    template <class TInputSource>
    TString ToOutString(const TInputSource& inputSource, bool full = false) const {
        Y_UNUSED(full);
        typedef typename TInputSource::value_type TSymbol;
        TString res;
        TStringOutput out(res);
        out << RuleName;
        if (!NamedTokens.empty()) {
            out << ":";
            TVector<TSymbol> symbols;
            ExtractMatched(inputSource, symbols);
            for (TNamedTokens::const_iterator it = NamedTokens.begin(); it != NamedTokens.end(); ++it) {
                out << ' ' << it->first << "(" << symbols[it->second]->GetText() << ")";
            }
        }
        return res;
    }
};

typedef TIntrusivePtr<TTokenLogicResult> TTokenLogicResultPtr;
typedef TVector<TTokenLogicResultPtr> TTokenLogicResults;

} // NTokenLogic

namespace NSolveAmbig {

template <>
struct TOccurrenceTraits<NTokenLogic::TTokenLogicResult>: public TOccurrenceTraits<NMatcher::TResultBase> {
    inline static double GetWeight(const NTokenLogic::TTokenLogicResult& r) {
        return r.GetWeight();
    }
};

} // NSolveAmbig

Y_DECLARE_OUT_SPEC(inline, NTokenLogic::TNamedTokens, out, nt) {
    out << "{";
    for (NTokenLogic::TNamedTokens::const_iterator i = nt.begin(); i != nt.end(); ++i) {
        if (i != nt.begin())
            out << ",";
        out << i->first << "->" << i->second;
    }
    out << "}";
}
