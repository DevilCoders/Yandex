#pragma once

#include "shingler.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <utility>

namespace NCrossing {
    struct TLineToken {
        ui32 Line;
        ui32 Words;
        ui32 Begin;
        ui32 End;

        TLineToken()
            : Line(0)
            , Words(0)
            , Begin(0)
            , End(0)
        {
        }

        TLineToken(ui32 line, ui32 words, ui32 begin, ui32 end)
            : Line(line)
            , Words(words)
            , Begin(begin)
            , End(end)
        {
        }
    };

    struct TLineTokenLess {
        bool operator()(const TLineToken& a, const TLineToken& b) const {
            if (a.Line < b.Line)
                return true;
            if (a.Line > b.Line)
                return false;
            return a.Begin < b.Begin;
        }
    };

    typedef TVector<TLineToken> TLineTokens;
    typedef std::pair<TLineToken, TLineToken> TCrossToken;
    typedef TVector<TCrossToken> TCrossing;

    template <typename TSelect>
    struct TCrossTokenCompare {
        int operator()(const TCrossToken& a, const TCrossToken& b) const {
            if (a.first.Line < b.first.Line)
                return -1;
            if (a.first.Line > b.first.Line)
                return 1;
            if (a.second.Line < b.second.Line)
                return -1;
            if (a.second.Line > b.second.Line)
                return 1;
            if (TSelect::Do(a.first) < TSelect::Do(b.first))
                return -1;
            if (TSelect::Do(a.first) > TSelect::Do(b.first))
                return 1;
            if (TSelect::Do(a.second) < TSelect::Do(b.second))
                return -1;
            if (TSelect::Do(a.second) > TSelect::Do(b.second))
                return 1;
            return 0;
        }
    };

    struct TByBegin {
        static ui32 Do(const TLineToken& t) {
            return t.Begin;
        }
    };

    struct TByEnd {
        static ui32 Do(const TLineToken& t) {
            return t.End;
        }
    };

    template <typename TBy>
    struct TCrossTokenLess {
        bool operator()(const TCrossToken& a, const TCrossToken& b) const {
            return TCrossTokenCompare<TBy>()(a, b) < 0;
        }
    };

    template <typename T>
    struct TWord {
        T Hash;
        ui32 Line;
        ui32 Pos;
        ui32 Len;

        TWord()
            : Hash(0)
            , Line(0)
            , Pos(0)
            , Len(0)
        {
        }

        TWord(T hash, ui32 line, ui32 pos, ui32 len)
            : Hash(hash)
            , Line(line)
            , Pos(pos)
            , Len(len)
        {
        }

        operator T() const {
            return Hash;
        }

        bool operator<(const TWord<T>& w) const {
            if (this->Hash < w.Hash)
                return true;
            if (this->Hash > w.Hash)
                return false;
            if (this->Line < w.Line)
                return true;
            if (this->Line > w.Line)
                return false;
            return this->Pos < w.Pos;
        }
    };

    void Expand(TCrossing& ranges, const TCrossing& tokens);

    template <typename T>
    void Build(TCrossing& result, const TVector<TWord<T>>& text) {
        // First: transform tokens matching by hash to crossing tokens
        TCrossing tokens;
        {
            TLineTokens curr;
            T prevHash = T();

            typename TVector<TWord<T>>::const_iterator i = text.begin(), e = text.end();
            for (; i != e; ++i) {
                TLineToken currTok(i->Line, 1, i->Pos, i->Pos + i->Len);
                if (i->Hash == prevHash) {
                    TLineTokens::const_iterator p = curr.begin(), q = curr.end();
                    for (; p != q; ++p) {
                        if (i->Line == p->Line)
                            continue;
                        tokens.push_back(std::make_pair(currTok, *p));
                        tokens.push_back(std::make_pair(*p, currTok));
                    }
                } else if (prevHash < i->Hash) {
                    TLineTokens tmp;
                    curr.swap(tmp);
                } else
                    ythrow yexception() << "Input words are not sorted by hash";

                curr.push_back(currTok);
                prevHash = i->Hash;
            }
        }

        // Second: sort crossing tokens by line and begin position
        Sort(tokens.begin(), tokens.end(), TCrossTokenLess<TByBegin>());

        // Third: expand crossing tokens to maximum crossing ranges
        TCrossing ranges;
        Expand(ranges, tokens);

        // Fourth: sort result by line and end position
        Sort(ranges.begin(), ranges.end(), TCrossTokenLess<TByEnd>());

        // Fifth: assign result
        DoSwap(ranges, result);
    }

    template <typename TResult>
    void Annotate(TResult& result, const TCrossing& ranges) {
        TCrossToken tok;
        TCrossing::const_iterator i = ranges.begin(), e = ranges.end();
        bool run = true;
        while (run) {
            run = i != e;
            if (!run || TCrossTokenLess<TByEnd>()(tok, *i)) {
                if (tok.first.Begin != tok.first.End)
                    result.Do(tok.first, tok.second, !run);
            }
            if (!run)
                break;
            tok = *i;
            ++i;
        }
    }
}

// text must be sorted
template <typename TResult, typename T>
void FindCrossing(TResult& result, const TVector<NCrossing::TWord<T>>& text) {
    NCrossing::TCrossing ranges;
    NCrossing::Build(ranges, text);
    NCrossing::Annotate(result, ranges);
}
