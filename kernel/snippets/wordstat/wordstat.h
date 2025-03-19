#pragma once

#include <kernel/snippets/span/span.h>
#include <util/generic/ptr.h>

namespace NSnippets
{
    class TQueryy;
    class TSentsMatchInfo;
    class TSnip;
    class TWordStatData;

    class TWordStat
    {
    private:
        class TImpl;

        THolder<TImpl> Impl;

    public:
        TWordStat(const TQueryy& query, const TSentsMatchInfo& info, const TSnip* preloadSnip = nullptr);
        ~TWordStat();

        const TWordStatData& Data(bool withPreload = false) const;
        const TWordStatData& ZeroData(bool withPreload = false) const;

        bool DoCheck(int i, int j);

        void SetSpan(int i, int j);
        void AddSpan(int i, int j);

        size_t GetSpansCount() const;
    };
}
