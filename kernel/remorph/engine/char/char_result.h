#pragma once

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/proc_base/result_base.h>

#include <library/cpp/solve_ambig/occurrence.h>

#include <util/stream/str.h>
#include <util/stream/trace.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NReMorph {

struct TCharResult: public NMatcher::TResultBase {
    // All submatches have positions relative to the MatchTrack sequence, not to the original sequence!!
    // Use special methods to extract corresponding input symbols.
    NRemorph::TNamedSubmatches NamedSubmatches;

    TCharResult(const std::pair<TString, double>& ruleInfo, const std::pair<size_t, size_t>& region, const NRemorph::TMatchTrack& track)
        : NMatcher::TResultBase(ruleInfo.first, ruleInfo.second, region, track)
    {
    }

    void GetNamedRanges(NRemorph::TNamedSubmatches& ranges) const override {
        ranges.insert(NamedSubmatches.begin(), NamedSubmatches.end());
    }

    inline double GetWeight() const {
        // The more named submatches - the better weight.
        return NMatcher::TResultBase::GetWeight()
            * (NamedSubmatches.size() > 0 ? NamedSubmatches.size() : 1);
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    TString ToDebugString(int level, const TInputSource& inputSource) const {
        typedef typename TInputSource::value_type TSymbol;
        TString res;
        TStringOutput out(res);
        out << "rule=" << RuleName << ", pr=" << Priority << ", src=[" << WholeExpr.first << ',' << WholeExpr.second << ')';
        if (int(TRACE_DETAIL) <= level) {
            out << ", submatches: " << NamedSubmatches;
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

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    TString ToOutString(const TInputSource& inputSource, bool) const {
        typedef typename TInputSource::value_type TSymbol;
        TString res;
        TStringOutput out(res);
        TVector<TSymbol> symbols;
        ExtractMatched(inputSource, symbols);
        out << RuleName << ":";
        if (!NamedSubmatches.empty()) {
            for (NRemorph::TNamedSubmatches::const_iterator it = NamedSubmatches.begin(); it != NamedSubmatches.end(); ++it) {
                if (!it->second.IsEmpty()) {
                    out << ' ' << it->first << "("
                        << NSymbol::ToString(symbols.begin() + it->second.first, symbols.begin() + it->second.second)
                        << ")";
                }
            }
        }
        return res;
    }

    inline void DbgPrint(IOutputStream& s) const {
        s << RuleName << "," << NamedSubmatches;
    }
};

typedef TIntrusivePtr<TCharResult> TCharResultPtr;
typedef TVector<TCharResultPtr> TCharResults;

} // NReMorph

namespace NSolveAmbig {

template <>
struct TOccurrenceTraits<NReMorph::TCharResult>: public TOccurrenceTraits<NMatcher::TResultBase> {
    inline static double GetWeight(const NReMorph::TCharResult& r) {
        return r.GetWeight();
    }
};

} // NSolveAmbig
