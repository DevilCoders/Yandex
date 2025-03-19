#pragma once

#include "word_input_symbol.h"

#include <kernel/remorph/misc/ansi_escape/ansi_escape.h>
#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/tokenizer/tokenizer.h>

#include <library/cpp/solve_ambig/unique_results.h>
#include <library/cpp/solve_ambig/find_solutions.h>
#include <library/cpp/solve_ambig/rank.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/typetraits.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/strip.h>
#include <util/system/mutex.h>
#include <utility>

namespace NText {

enum EPrintVerbosity {
    PV_SHORT,
    PV_FULL,
    PV_TEXTONLY,
    PV_GREP,
};

enum EPrintResultMode {
    PRM_RESULTS,
    PRM_SOLUTIONS,
};

template <class TSymbolPtr>
inline std::pair<size_t, size_t> GetPos(const TVector<TSymbolPtr>& symbols, const NFact::TFact& fact) {
    return ::std::make_pair(symbols[fact.GetSrcPos().first]->GetSentencePos().first,
                      symbols[fact.GetSrcPos().second - 1]->GetSentencePos().second);
}

template <class TInputSource, class TResult>
inline std::pair<size_t, size_t> GetPos(const TInputSource& inputSource, const TResult& result) {
    return ::std::make_pair(result.GetMatchedSymbol(inputSource, 0)->GetSentencePos().first,
                      result.GetMatchedSymbol(inputSource, result.GetMatchedCount() - 1)->GetSentencePos().second);
}

inline bool TrailingNewline(const TWtringBuf& text) {
    return !text.empty() && text.back() == static_cast<wchar16>('\n');
}

template <bool MultiThreaded>
class TResultPrinter {
private:
    const static NReMorph::EColorCode COLOR = NReMorph::CC_RED;

    typedef std::conditional_t<MultiThreaded, TMutex, TFakeMutex> TMutexType;
    TMutexType Mutex;
    IOutputStream& Output;
    bool PrintUnmatched;
    EPrintVerbosity PrintVerbosity;
    EPrintResultMode ResultMode;
    NSolveAmbig::TRankMethod SolutionRankMethod;
    bool InvertGrep;
    bool Colorized;

    TString CurrentDoc;
    size_t Count;

    Y_HAS_MEMBER(GetSentencePos);

    template <bool val>
    struct TUseSentencePos {};

public:
    TResultPrinter(IOutputStream& out)
        : Output(out)
        , PrintUnmatched(false)
        , PrintVerbosity(PV_SHORT)
        , ResultMode(PRM_RESULTS)
        , SolutionRankMethod(NSolveAmbig::DefaultRankMethod())
        , InvertGrep(false)
        , Colorized(false)
        , Count(0)
    {
    }

    inline void SetCurrentDoc(const TString& doc) {
        CurrentDoc = doc;
        Count = 0;
    }

    inline const TString& GetCurrentDoc() const {
        return CurrentDoc;
    }

    inline void SetPrintUnmatched(bool print) {
        PrintUnmatched = print;
    }

    inline bool GetPrintUnmatched() const {
        return PrintUnmatched;
    }

    inline void SetPrintVerbosity(EPrintVerbosity v) {
        PrintVerbosity = v;
    }

    inline EPrintVerbosity GetPrintVerbosity() const {
        return PrintVerbosity;
    }

    inline void SetPrintResultMode(EPrintResultMode mode) {
        ResultMode = mode;
    }

    inline EPrintResultMode GetPrintResultMode() const {
        return ResultMode;
    }

    inline void SetSolutionRankMethod(const NSolveAmbig::TRankMethod& solutionRankMethod) {
        SolutionRankMethod = solutionRankMethod;
    }

    inline const NSolveAmbig::TRankMethod& GetSolutionRankMethod() const {
        return SolutionRankMethod;
    }

    inline void SetInvertGrep(bool invertGrep) {
        InvertGrep = invertGrep;
    }

    inline bool GetInvertGrep() const {
        return InvertGrep;
    }

    inline size_t GetResultCount() const {
        return Count;
    }

    inline void SetColorized(bool colorized) {
        Colorized = colorized;
    }

    inline bool GetColorized() const {
        return Colorized;
    }

    inline void PrintSent(const NToken::TSentenceInfo& sent) const {
        if (PV_TEXTONLY == PrintVerbosity) {
            Output << StripString(sent.Text);
        } else {
            if (!CurrentDoc.empty()) {
                Output << CurrentDoc << '\t';
            }
            if (sent.Pos.first != sent.Pos.second) {
                Output << sent.Pos.first << '\t' << sent.Pos.second << '\t';
            }
            Output << StripString(sent.Text) << Endl;
        }
    }

    template <class TInputSource, class TResultPtr>
    inline void PrintGreped(const NToken::TSentenceInfo& sent, const TInputSource& inputSource,
                            TVector<TResultPtr>& results) const {
        typedef typename TInputSource::value_type TSymbolPtr;
        PrintGreped(sent, inputSource, results, TUseSentencePos<THasGetSentencePos<typename TSymbolPtr::TValueType>::value>());
    }

    template <class TInputSource, class TResultPtr>
    inline void PrintGreped(const NToken::TSentenceInfo& sent, const TInputSource& inputSource,
                            TVector<TResultPtr>& results, const TUseSentencePos<true>&) const {

        if (Colorized && !results.empty()) {
            TVector<std::pair<size_t, size_t>> ranges(1, GetPos(inputSource, *results[0]));
            ranges.reserve(results.size());

            for (size_t i = 1; i < results.size(); ++i) {
                std::pair<size_t, size_t> range = GetPos(inputSource, *results[i]);
                Y_ASSERT(range >= ranges.back());
                if (range.first <= ranges.back().second) {
                    if (range.second > ranges.back().second) {
                        ranges.back().second = range.second;
                    }
                } else {
                    ranges.push_back(range);
                }
            }

            size_t printed = 0;
            for (const auto& range: ranges) {
                if (range.first > printed) {
                    Output << TWtringBuf(sent.Text, printed, range.first - printed);
                }
                Output << NReMorph::AnsiSgrColor(COLOR, true) <<
                    TWtringBuf(sent.Text, range.first, range.second - range.first) <<
                    NReMorph::AnsiSgrColorReset();
                printed = range.second;
            }
            if (printed < sent.Text.size()) {
                Output << TWtringBuf(sent.Text, printed, sent.Text.size() - printed);
            }
        } else {
            Output << sent.Text;
        }

        if (!TrailingNewline(sent.Text)) {
            Output << Endl;
        }
    }

    template <class TInputSource, class TResultPtr>
    inline void PrintGreped(const NToken::TSentenceInfo& sent, const TInputSource& inputSource,
                            TVector<TResultPtr>& results, const TUseSentencePos<false>&) const {
        Y_UNUSED(inputSource);
        Y_UNUSED(results);
        Output << sent.Text;
        if (!TrailingNewline(sent.Text)) {
            Output << Endl;
        }
    }

    template <class TInputSource, class TResult>
    inline void PrintResult(const NToken::TSentenceInfo& sent, const TInputSource& inputSource, const TResult& res) const {
        typedef typename TInputSource::value_type TSymbolPtr;
        PrintResult(sent, inputSource, res, TUseSentencePos<THasGetSentencePos<typename TSymbolPtr::TValueType>::value>());
    }

    template <class TSymbolPtr>
    inline void PrintResult(const NToken::TSentenceInfo& sent, const TVector<TSymbolPtr>& symbols, const NFact::TFact& fact,
        const TUseSentencePos<true>&) const {
        Y_ASSERT(fact.GetSrcPos().first < symbols.size());
        Y_ASSERT(fact.GetSrcPos().second <= symbols.size());
        std::pair<size_t, size_t> pos = GetPos(symbols, fact);
        switch (PrintVerbosity) {
        case PV_SHORT:
        case PV_FULL:
            Output << '\t' << (sent.Pos.first + pos.first)
                << '\t' << (sent.Pos.first + pos.second)
                << '\t' << fact.ToString(PV_SHORT == PrintVerbosity) << Endl;
            break;
        case PV_TEXTONLY:
            Output << '\t' << sent.Text.substr(pos.first, pos.second - pos.first);
            break;
        default:
            Y_FAIL("Unsupported print field mode");
        }
    }

    template <class TSymbolPtr>
    inline void PrintResult(const NToken::TSentenceInfo&, const TVector<TSymbolPtr>& symbols, const NFact::TFact& fact,
        const TUseSentencePos<false>&) const {
        Y_ASSERT(fact.GetSrcPos().first < symbols.size());
        Y_ASSERT(fact.GetSrcPos().second <= symbols.size());
        switch (PrintVerbosity) {
        case PV_SHORT:
        case PV_FULL:
            Output << '\t' << fact.ToString(PV_SHORT == PrintVerbosity) << Endl;
            break;
        case PV_TEXTONLY:
            Output << '\t' << NSymbol::ToString(symbols.begin() + fact.GetSrcPos().first, symbols.begin() + fact.GetSrcPos().second);
            break;
        default:
            Y_FAIL("Unsupported print field mode");
        }
    }

    template <class TInputSource, class TResult>
    inline void PrintResult(const NToken::TSentenceInfo& sent, const TInputSource& inputSource, const TResult& res,
        const TUseSentencePos<true>&) const {
        switch (PrintVerbosity) {
        case PV_SHORT:
        case PV_FULL:
            Output << '\t' << res.ToOutString(inputSource, PV_FULL == PrintVerbosity) << Endl;
            break;
        case PV_TEXTONLY:
            {
                std::pair<size_t, size_t> pos = GetPos(inputSource, res);
                Output << '\t' << sent.Text.substr(pos.first, pos.second - pos.first);
            }
            break;
        default:
            Y_FAIL("Unsupported print field mode");
        }
    }

    template <class TInputSource, class TResult>
    inline void PrintResult(const NToken::TSentenceInfo&, const TInputSource& inputSource, const TResult& res,
        const TUseSentencePos<false>&) const {
        switch (PrintVerbosity) {
        case PV_SHORT:
        case PV_FULL:
            Output << '\t' << res.ToOutString(inputSource, PV_FULL == PrintVerbosity) << Endl;
            break;
        case PV_TEXTONLY:
            {
                TVector<typename TInputSource::value_type> symbols;
                res.ExtractMatched(inputSource, symbols);
                Output << '\t' << NSymbol::ToString(symbols);
            }
            break;
        default:
            Y_FAIL("Unsupported print field mode");
        }
    }

    template <class TInputSource, class TResultPtr>
    inline void PrintSolutions(const NToken::TSentenceInfo& sent, const TInputSource& inputSource, TVector<TResultPtr>& results, const NSolveAmbig::TRankMethod& rankMethod = NSolveAmbig::DefaultRankMethod()) const {
        NSolveAmbig::MakeUniqueResults(results);
        TVector<NSolveAmbig::TSolutionPtr> solutions;
        bool all = NSolveAmbig::FindAllSolutions(results, solutions, rankMethod);
        if (!all) {
            Output << "First " << solutions.size() << " solutions:" << Endl;
        }
        for (size_t i = 0; i < solutions.size(); ++i) {
            Output << "Solution " << ToString(i)
                << " (coverage=" << solutions[i]->Coverage
                << ", weight=" << solutions[i]->Weight << "):" << Endl;
            for (size_t j = 0; j < solutions[i]->Positions.size(); ++j) {
                PrintResult(sent, inputSource, *results[solutions[i]->Positions[j]]);
            }
            if (PV_TEXTONLY == PrintVerbosity) {
                Output << Endl;
            }
        }
    }

    template <class TInputSource, class TResultPtr>
    inline void PrintResults(const NToken::TSentenceInfo& sent, const TInputSource& inputSource, TVector<TResultPtr>& results) {
        bool needEndLine = false;

        if (PV_GREP == PrintVerbosity) {
            if (results.empty() == InvertGrep) {
                PrintGreped(sent, inputSource, results);
            }
        } else if (!results.empty() || PrintUnmatched) {
            PrintSent(sent);
            needEndLine = PV_TEXTONLY == PrintVerbosity;

            if (!results.empty() ) {
                if (PRM_SOLUTIONS == ResultMode) {
                    if (needEndLine) {
                        Output << Endl;
                        needEndLine = false;
                    }
                    PrintSolutions(sent, inputSource, results, SolutionRankMethod);
                } else {
                    for (size_t i = 0; i < results.size(); ++i) {
                        PrintResult(sent, inputSource, *results[i]);
                    }
                }
            }
        }

        if (needEndLine) {
            Output << Endl;
        }
        Count += results.size();
    }

    template <class TInputSource, class TResultPtr>
    inline void operator()(const NToken::TSentenceInfo& sent, const TInputSource& inputSource, TVector<TResultPtr>& results) {
        TGuard<TMutexType> g(Mutex);
        PrintResults(sent, inputSource, results);
    }
};

} // NText
