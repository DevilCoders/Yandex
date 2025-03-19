#pragma once

#include <library/cpp/charset/codepage.h>
#include <kernel/remorph/core/debug.h>
#include <kernel/remorph/core/executor.h>
#include <kernel/remorph/core/literal.h>
#include <kernel/remorph/core/submatch.h>
#include <kernel/remorph/core/tokens.h>

#include <util/charset/wide.h>
#include <util/generic/bitmap.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/string/vector.h>
#include <util/ysaveload.h>

namespace NRemorph {

#define DBGOM GetDebugOutMATCH()

struct TLiteralTableWtrokaImpl {
    TVector<TUtf16String> Strings;
    typedef THashMap<TUtf16String, TLiteral> TTable;
    TTable Table;

    TLiteral Add(const TUtf16String& s) {
        TTable::iterator i = Table.find(s);
        if (i == Table.end()) {
            TLiteral l(Strings.size(), TLiteral::Ordinal);
            Strings.push_back(s);
            Table.insert(std::make_pair(s, l));
            return l;
        }
        return i->second;
    }
    TLiteral Get(const TUtf16String& s) {
        TTable::iterator i = Table.find(s);
        return i->second;
    }
    TString ToString(TLiteral l) const {
        return WideToUTF8(Strings[l.GetId()]);
    }
    TLiteralId GetPriority(TLiteral) const {
        return 0;
    }
    void Save(IOutputStream* out) const {
        ::Save(out, Strings);
    }
    void Load(IInputStream* in) {
        ::Load(in, Strings);
    }
    TLiteralId Size() const {
        return Strings.size();
    }

    bool IsEqual(TLiteral l, const TUtf16String& w, TDynBitMap&) const {
        return Strings[l.GetId()] == w;
    }
};

struct TLiteralTableAsciiImpl {
    TString ToString(TLiteral l) const {
        return TString(1, (char)l.GetId());
    }
    TLiteralId GetPriority(TLiteral) const {
        return 0;
    }
    TLiteralId Size() const {
        return 128;
    }
    static bool IsEqual(TLiteral l, char c_, TDynBitMap&) {
        char c = csYandex.ToLower(c_);
        return  c == csYandex.ToLower((char)l.GetId());
    }
};

typedef TLiteralTable<TLiteralTableWtrokaImpl> TLiteralTableWtroka;
typedef TVector<TUtf16String> TVectorSymbols;
typedef TLiteralTable<TLiteralTableAsciiImpl> TLiteralTableAscii;


void Parse(TLiteralTableWtroka& lt, TVectorTokens& out, const TUtf16String& s);
void Parse(const TUtf16String& s, TVectorSymbols& out);
void Parse(TVectorTokens& out, const TString& s);


struct TFirstMatch {
    TMatchInfoPtr Result;

    inline operator bool() const {
        return !Result;
    }

    template <class TSymbolIterator>
    inline void operator()(const TMatchState<TSymbolIterator>& state) {
        if (state.GetIter().AtEnd()) {
            TVector<TMatchInfoPtr> matches = state.CreateMatchInfos();
            if (!matches.empty())
                Result = matches.front();
        }
    }
};

struct TAllMatches {
    TVector<TMatchInfoPtr>& Results;

    TAllMatches(TVector<TMatchInfoPtr>& res)
        : Results(res)
    {
    }

    inline operator bool() const {
        return true;
    }

    template <class TSymbolIterator>
    inline void operator()(const TMatchState<TSymbolIterator>& state) const {
        if (state.GetIter().AtEnd()) {
            TVector<TMatchInfoPtr> matches = state.CreateMatchInfos();
            Results.insert(Results.end(), matches.begin(), matches.end());
        }
    }
};

struct TAllSearches {
    TVector<TMatchInfoPtr>& Results;
    size_t Start;

    TAllSearches(TVector<TMatchInfoPtr>& res)
        : Results(res)
        , Start(0)
    {
    }

    inline void SetStart(size_t s) {
        Start = s;
    }

    inline operator bool() const {
        return true;
    }

    template <class TSymbolIterator>
    inline void operator()(const TMatchState<TSymbolIterator>& state) const {
        TVector<TMatchInfoPtr> matches = state.CreateMatchInfos();
        TSubmatch wholeExpr;
        wholeExpr.first = Start;
        wholeExpr.second = state.GetIter().GetCurPos();
        for (TVector<TMatchInfoPtr>::iterator i = matches.begin(); i != matches.end(); ++i)
            i->Get()->NamedSubmatches.insert(std::make_pair("$$", wholeExpr));
        Results.insert(Results.end(), matches.begin(), matches.end());
    }
};

template <class TLiteralTable, class TSymbolIterator>
inline TMatchInfoPtr MatchText(const TLiteralTable& lt, const TDFA& dfa, const TSymbolIterator& begin) {
    TFirstMatch collector;
    SearchFrom(lt, dfa, begin, collector);
    return collector.Result;
}

template <class TLiteralTable, class TSymbolIterator>
inline void MatchAllText(const TLiteralTable& lt, const TDFA& dfa, const TSymbolIterator& begin, TVector<TMatchInfoPtr>& matches) {
    TAllMatches collector(matches);
    SearchFrom(lt, dfa, begin, collector);
}

template <class TLiteralTable, class TSymbolIterator>
inline void SearchAllText(const TLiteralTable& lt, const TDFA& dfa, const TSymbolIterator& begin, TVector<TMatchInfoPtr>& matches) {
    TAllSearches collector(matches);
    TVector<TSymbolIterator> cur(1, begin);
    TVector<TSymbolIterator> next;
    while (!cur.empty()) {
        next.clear();
        for (typename TVector<TSymbolIterator>::const_iterator i = cur.begin(); i != cur.end(); ++i) {
            SearchFrom(lt, dfa, *i, collector);
            const size_t cnt = i->GetNextCount();
            for (size_t nit = 0; nit < cnt; ++nit) {
                next.push_back(i->GetNext(nit));
            }
        }
        DoSwap(next, cur);
    }
}

} // NRemorph
