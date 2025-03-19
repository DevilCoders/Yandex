#pragma once

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/proc_base/result_base.h>
#include <kernel/remorph/input/input_symbol_util.h>

#include <library/cpp/solve_ambig/occ_traits.h>

#include <util/stream/str.h>
#include <util/stream/trace.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NReMorph {

struct TMatchResult: public NMatcher::TResultBase {
    // All submatches have positions relative to the MatchTrack sequence, not to the original sequence!!
    // Use special methods to extract corresponding input symbols.
    NRemorph::TMatchInfoPtr Result;

    TMatchResult(const NRemorph::TMatchInfoPtr& result, const std::pair<TString, double>& ruleInfo, const std::pair<size_t, size_t>& region, const NRemorph::TMatchTrack& track)
        : NMatcher::TResultBase(ruleInfo.first, ruleInfo.second, region, track)
        , Result(result)
    {
    }

    void GetNamedRanges(NRemorph::TNamedSubmatches& ranges) const override {
        ranges.insert(Result->NamedSubmatches.begin(), Result->NamedSubmatches.end());
    }

    inline double GetWeight() const {
        // The more named submatches - the better weight.
        return NMatcher::TResultBase::GetWeight()
            * (Result->NamedSubmatches.size() > 0 ? Result->NamedSubmatches.size() : 1);
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource, class TSourceVector, class TDestVector>
    void FillVector(const TInputSource& inputSource, TDestVector& dest, const TSourceVector& source, const NRemorph::TSubmatch& pos) const {
        typedef typename TInputSource::value_type TSymbol;
        TVector<TSymbol> subSymbols;
        ExtractMatched(inputSource, pos, subSymbols);
        for (typename TVector<TSymbol>::const_iterator i = subSymbols.begin(); i != subSymbols.end(); ++i) {
            const std::pair<size_t, size_t>& sourcePos = i->Get()->GetSourcePos();
            for (size_t j = sourcePos.first; j < sourcePos.second; ++j)
                dest.push_back(source[j]);
        }
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource, class TSourceVector, class TDestVector>
    inline void FillVector(const TInputSource& inputSource, TDestVector& dest, const TSourceVector& source, TStringBuf submatchName) const {
        NRemorph::TNamedSubmatches::const_iterator it = Result->NamedSubmatches.find(submatchName);
        if (it != Result->NamedSubmatches.end()) {
            FillVector(inputSource, dest, source, it->second);
        }
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource, class TSourceVector, class TDestVector>
    inline void FillVector(const TInputSource& inputSource, TDestVector& dest, const TSourceVector& source, size_t submatchIndex) const {
        Y_ASSERT(submatchIndex < Result->Submatches.size());
        FillVector(inputSource, dest, source, Result->Submatches[submatchIndex]);
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    TString ToDebugString(int level, const TInputSource& inputSource) const {
        typedef typename TInputSource::value_type TSymbol;
        TString res;
        TStringOutput out(res);
        out << "rule=" << RuleName << ", pr=" << Priority << ", src=[" << WholeExpr.first << ',' << WholeExpr.second << ')';
        if (int(TRACE_DETAIL) <= level) {
            out << ", submatches: " << Result->NamedSubmatches;
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
    TString ToOutString(const TInputSource& inputSource, bool unnamed = false) const {
        typedef typename TInputSource::value_type TSymbol;
        TString res;
        TStringOutput out(res);
        TVector<TSymbol> symbols;
        ExtractMatched(inputSource, symbols);
        out << RuleName << ":";
        if (!Result->NamedSubmatches.empty()) {
            for (NRemorph::TNamedSubmatches::const_iterator it = Result->NamedSubmatches.begin();
                it != Result->NamedSubmatches.end(); ++it) {
                if (!it->second.IsEmpty()) {
                    out << ' ' << it->first << "("
                        << NSymbol::ToString(symbols.begin() + it->second.first, symbols.begin() + it->second.second)
                        << ")";
                }
            }
        }
        if (unnamed && !Result->Submatches.empty()) {
            for (size_t n = 0; n < Result->Submatches.size(); ++n) {
                if (!Result->Submatches[n].IsEmpty()) {
                    out << " <" << n << ">("
                        << NSymbol::ToString(symbols.begin() + Result->Submatches[n].first, symbols.begin() + Result->Submatches[n].second)
                        << ")";
                }
            }
        }
        return res;
    }

    inline void DbgPrint(IOutputStream& s) const {
        s << RuleName << "," << Result->Submatches << "," << Result->NamedSubmatches;
    }
};

typedef TIntrusivePtr<TMatchResult> TMatchResultPtr;
typedef TVector<TMatchResultPtr> TMatchResults;

} // NReMorph

namespace NSolveAmbig {

template <>
struct TOccurrenceTraits<NReMorph::TMatchResult>: public TOccurrenceTraits<NMatcher::TResultBase> {
    inline static double GetWeight(const NReMorph::TMatchResult& r) {
        return r.GetWeight();
    }
};

} // NSolveAmbig
