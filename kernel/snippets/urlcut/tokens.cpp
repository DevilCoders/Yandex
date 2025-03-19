#include "tokens.h"
#include "consts.h"
#include "hilited_string.h"

#include <util/generic/utility.h>

namespace NUrlCutter {
    TSpan::TSpan(ui32 pos, size_t len, bool isStop)
        : Begin(ui32(pos - len + 1))
        , End(pos + 1)
        , IsStop(isStop || len <= 2)
    {
    }

    bool TSpan::TryMerge(const TSpan& next) {
        if (next.Begin <= End) {
            Begin = Min(Begin, next.Begin);
            End = Max(End, next.End);
            IsStop &= next.IsStop;
            return true;
        }
        return false;
    }

    TTokenInfo::TTokenInfo(const TUtf16String& token, ETokenType tt, EPathType pt,
            const TMatcherSearcher::TSearchResult* sr)
        : Token(token)
        , TType(tt)
        , PType(pt)
        , Matches()
        , Len(Token.size())
        , TStatus(TS_NONE)
        , AddRevFreq(0)
        , Gray(0)
        , HiliteSpans()
    {
        if (auto g = GrayList.find(Token); g != GrayList.end()) {
            Gray = g->second;
        }
        if (sr) {
            //разбираем результаты поиска подстрок
            THashSet<TWtringBuf> unique;
            for (const auto& item: *sr) {
                //при расчете веса, нужно каждое слово запроса учитывать только 1 раз
                if (unique.insert(item.second.Word).second) {
                    Matches.push_back(item.second);
                }
                //мержим span'ы подсветки
                if (HiliteSpans.empty()) {
                    HiliteSpans.emplace_back(item.first, item.second.Len, item.second.IsStop);
                } else {
                    TSpan span(item.first, item.second.Len, item.second.IsStop);
                    auto nit = HiliteSpans.rbegin();
                    if (nit->TryMerge(span)) {
                        auto it = nit++;
                        while(nit != HiliteSpans.rend() && nit->TryMerge(*it)) {
                            it = nit++;
                            HiliteSpans.pop_back();
                        }
                    } else {
                        HiliteSpans.push_back(std::move(span));
                    }
                }
            }
        }
    }

    i64 TTokenInfo::GetFreq(THashSet<TUtf16String>& used, bool wGray) const {
        //считаем вес без учета использованных слов
        i64 freq(0);
        for (auto it = Matches.begin(); it != Matches.end(); ++it) {
            if (!used.contains(it->Word)) {
                freq += it->RevFreq;
            } else {
                freq += 15;
            }
        }
        if (!freq && TType == TT_DATE) {
            freq += 1000;
        }
        if (wGray)
            freq += Gray;
        return freq + AddRevFreq;
    }

    void TTokenInfo::FillUsed(THashSet<TUtf16String>& used) const {
        for (auto it = Matches.begin(); it != Matches.end(); ++it) {
            used.insert(it->Word);
        }
    }
    void TTokenInfo::SetStatus(ETokenStatus tstatus, THashSet<TUtf16String>& used) {
        TStatus = tstatus;
        if (tstatus == TS_USED) {
            FillUsed(used);
        }
    }

    bool TTokenInfo::Update(TTokenInfo& prev, bool wasSp,
                            const THashSet<TUtf16String>& DynamicStopWords) {
        if (HiliteSpans.empty() || prev.HiliteSpans.empty())
            return false;

        TSpan& ps = prev.HiliteSpans.back();
        TSpan& cs = HiliteSpans.front();

        if (!wasSp) {
            /*
            Удаляем метки стоп-слов, если два стоп-слова стоят рядом,
            и они на самом деле не являются стоп-словами (они
            просто короткие или были вычеркнуты из списка
            стоп-слов словами в пользовательском запросе).
            И небольшой чит - эти слова не должны совпадать.
            */
            if (Token != prev.Token && (ps.IsStop || cs.IsStop) &&
                prev.IsTotalHilite() && IsTotalHilite() &&
                (!DynamicStopWords.contains(prev.Token)) &&
                (!DynamicStopWords.contains(Token))) {
                ps.IsStop = false;
                cs.IsStop = false;
                return true;
            }
        }

        if (!wasSp) {
            if (ps.IsStop != cs.IsStop &&
                ((ps.IsStop && ps.End == prev.Len && cs.Begin == 0) ||
                 (cs.IsStop && cs.Begin == 0 && ps.End == prev.Len))) {
                ps.IsStop = false;
                cs.IsStop = false;
                return true;
            }
        } else {
            if (ps.IsStop != cs.IsStop &&
                ((ps.IsStop && ps.Begin == 0 && ps.End == prev.Len && cs.Begin == 0) ||
                 (cs.IsStop && cs.Begin == 0 && cs.End == Len && ps.End == prev.Len))) {
                ps.IsStop = false;
                cs.IsStop = false;
                return true;
            }
        }
        return false;
    }

    THilitedString TTokenInfo::GetHilitedToken() const {
        THilitedString hilitedToken;
        hilitedToken.String = Token;
        for (const TSpan& span : HiliteSpans) {
            if (!span.IsStop) {
                hilitedToken.SortedHilitedSpans.push_back({span.Begin, span.End});
            }
        }
        return hilitedToken;
    }

    void TTokenInfo::Relax() {
        if (HiliteSpans.size() != 1) {
            return;
        }
        auto it = HiliteSpans.begin();
        if (this->Len == 2 && this->Len == it->End - it->Begin &&
            StopWords.find(this->Token) == StopWords.end()) {
            it->IsStop = false;
        }
    }

    bool TTokenInfo::IsTotalHilite() const {
        if (HiliteSpans.size() != 1)
            return (false);
        return (HiliteSpans.front().Begin == 0 && HiliteSpans.front().End == Len);
    }
}
