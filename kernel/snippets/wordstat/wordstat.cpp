#include "wordstat.h"
#include "wordstat_data.h"
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/sent_match.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>

namespace NSnippets {

    class TWordStat::TImpl {
    public:
        const TQueryy& Query;

        const TSentsMatchInfo& Info;
        const TSnip* PreloadSnip;
        TSpans Spans;

        TWordStatData PreloadStat;
        TWordStatData Stat;
        TWordStatData PreloadZeroStat;
        TWordStatData ZeroStat;

    private:
        void HardReset()
        {
            Remove(Spans.begin(), Spans.end());
        }

        TSpanIt Remove(TSpanIt b, TSpanIt e)
        {
            for (TSpanIt it = b; it != e; ++it) {
                PreloadStat.UnPutSpan(Info, it->First, it->Last);
                Stat.UnPutSpan(Info, it->First, it->Last);
            }
            return Spans.erase(b, e);
        }

    public:
        TImpl(const TQueryy& query, const TSentsMatchInfo& info, const TSnip* preloadSnip = nullptr)
          : Query(query)
          , Info(info)
          , PreloadSnip(preloadSnip)
          , Spans()
          , PreloadStat(query, info.SentsInfo.MaxN)
          , Stat(query, info.SentsInfo.MaxN)
          , PreloadZeroStat(query, info.SentsInfo.MaxN)
          , ZeroStat(query, info.SentsInfo.MaxN)
        {
            if (PreloadSnip) {
                for (TSnip::TSnips::const_iterator it = PreloadSnip->Snips.begin(); it != PreloadSnip->Snips.end(); ++it) {
                    PreloadZeroStat.PutSpan(*it->GetSentsMatchInfo(), it->GetFirstWord(), it->GetLastWord(), 0, &Info);
                    PreloadStat.PutSpan(*it->GetSentsMatchInfo(), it->GetFirstWord(), it->GetLastWord(), 0, &Info);
                }
            }
        }

    public:

        const TWordStatData& Data(bool withPreload = false) const;
        const TWordStatData& ZeroData(bool withPreload = false) const;

        bool DoCheck(int i, int j);

        void SetSpan(int i, int j);
        void AddSpan(int i, int j);

        size_t GetSpansCount() const;
    };

    TWordStat::TWordStat(const TQueryy& query, const TSentsMatchInfo& info, const TSnip* preloadSnip)
      : Impl(new TImpl(query, info, preloadSnip))
    {
    }

    TWordStat::~TWordStat() {
    }

    const TWordStatData& TWordStat::Data(bool withPreload) const {
        return Impl->Data(withPreload);
    }

    const TWordStatData& TWordStat::ZeroData(bool withPreload) const {
        return Impl->ZeroData(withPreload);
    }

    bool TWordStat::DoCheck(int i, int j) {
        return Impl->DoCheck(i, j);
    }

    void TWordStat::SetSpan(int i, int j) {
        Impl->SetSpan(i, j);
    }

    void TWordStat::AddSpan(int i, int j) {
        Impl->AddSpan(i, j);
    }

    size_t TWordStat::GetSpansCount() const {
        return Impl->GetSpansCount();
    }

    const TWordStatData& TWordStat::TImpl::Data(bool withPreload) const
    {
        if (!PreloadSnip) {
            withPreload = false;
        }
        if (Spans.empty()) {
            return ZeroData(withPreload);
        } else {
            if (withPreload) {
                return PreloadStat;
            } else {
                return Stat;
            }
        }
    }

    const TWordStatData& TWordStat::TImpl::ZeroData(bool withPreload) const
    {
        if (withPreload && PreloadSnip) {
            return PreloadZeroStat;
        } else {
            return ZeroStat;
        }
    }

    bool TWordStat::TImpl::DoCheck(int i, int j)
    {
        TWordStatData zero(Query, Info.SentsInfo.MaxN);
        if (!zero.Check(Stat, Info, i, j)) {
            return false;
        }
        if (PreloadSnip) {
            TWordStatData zeroPreload(Query, Info.SentsInfo.MaxN);
            for (TSnip::TSnips::const_iterator it = PreloadSnip->Snips.begin(); it != PreloadSnip->Snips.end(); ++it) {
                zeroPreload.PutSpan(*it->GetSentsMatchInfo(), it->GetFirstWord(), it->GetLastWord(), 0, &Info);
            }
            return zeroPreload.Check(PreloadStat, Info, i, j);
        } else {
            return true;
        }
    }

    void TWordStat::TImpl::SetSpan(int i, int j)
    {
        TSpan s(i, j);

        size_t len = 0;
        TSpanIt b = Spans.begin();
        while (b != Spans.end() && s < *b) {
            len += b->Len();
            ++b;
        }
        TSpanIt e = b;
        while (e != Spans.end() && e->Intersects(s)) {
            len += s.UnLen(*e);
            ++e;
        }
        for (TSpanIt it = e; it != Spans.end(); ++it) {
            len += it->Len();
        }

        if(b == e || len > s.Len()) {
            HardReset();
            Stat.PutSpan(Info, s.First, s.Last);
            if (PreloadSnip) {
                PreloadStat.PutSpan(Info, s.First, s.Last);
            }
        } else {
            b = Remove(Spans.begin(), b);
            e = Remove(e, Spans.end());
            TSpan t = s;
            ui8 left = 0;
            while (b != e) {
                if (b->First < t.First) {
                    Stat.UnPutSpan(Info, b->First, t.First - 1, PSM_RIGHT);
                    if (PreloadSnip) {
                        PreloadStat.UnPutSpan(Info, b->First, t.First - 1, PSM_RIGHT);
                    }
                } else if (t.First < b->First) {
                    Stat.PutSpan(Info, t.First, b->First - 1, PSM_RIGHT | left);
                    if (PreloadSnip) {
                        PreloadStat.PutSpan(Info, t.First, b->First - 1, PSM_RIGHT | left);
                    }
                    left = 0;
                }
                if (b->Last <= t.Last) {
                    t.First = b->Last + 1;
                    left = PSM_LEFT;
                } else {
                    t.First = t.Last + 1;//говорит, что больше добаить нечего
                    Stat.UnPutSpan(Info, t.Last + 1, b->Last, PSM_LEFT);
                    if (PreloadSnip) {
                        PreloadStat.UnPutSpan(Info, t.Last + 1, b->Last, PSM_LEFT);
                    }
                }
                b = Spans.erase(b);
            }
            if (t.Len()) {
                Stat.PutSpan(Info, t.First, t.Last, PSM_LEFT);
                if (PreloadSnip) {
                    PreloadStat.PutSpan(Info, t.First, t.Last, PSM_LEFT);
                }
            }
        }
        Y_ASSERT(Info.Cfg.SkipWordstatDebug() || DoCheck(s.First, s.Last));
        Spans.push_back(s);
    }

    void TWordStat::TImpl::AddSpan(int i, int j)
    {
        TSpan s(i, j);
        TSpanIt it = Spans.begin();
        while (it != Spans.end() && *it < s)
            ++it;
        ui8 left = 0;
        while (s.Len() && it != Spans.end() && it->Intersects(s)) {
            if (s.First < it->First) {
                TSpan t(s.First, it->First - 1);
                Stat.PutSpan(Info, t.First, t.Last, left | PSM_RIGHT);
                if (PreloadSnip) {
                    PreloadStat.PutSpan(Info, t.First, t.Last, left | PSM_RIGHT);
                }
                Spans.insert(it, t);
            }
            if (s.Last <= it->Last) {
                s.First = s.Last + 1;
            } else {
                left = PSM_LEFT;
                s.First = it->Last + 1;
                ++it;
            }
        }
        if (s.Len()) {
            Stat.PutSpan(Info, s.First, s.Last, left);
            if (PreloadSnip) {
                PreloadStat.PutSpan(Info, s.First, s.Last, left);
            }
            Spans.insert(it, s);
        }
        TSpanIt nit = Spans.begin();
        it = nit++;

        for (; nit != Spans.end();) {
            if (it->LeftTo(*nit)) {
                Stat.PutPair(Info, it->Last, nit->First);
                if (PreloadSnip) {
                    PreloadStat.PutPair(Info, it->Last, nit->First);
                }
                it->Merge(*nit);
                nit = Spans.erase(nit);
            } else {
                ++it;
                ++nit;
            }
        }
    }

    size_t TWordStat::TImpl::GetSpansCount() const
    {
        return Spans.size();
    }
}
