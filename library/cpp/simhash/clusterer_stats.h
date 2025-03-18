#pragma once

#include <util/generic/vector.h>
#include <util/generic/map.h>

#include <util/system/rusage.h>

namespace NNearDuplicates {
    template <typename T>
    class TBaseStat {
    public:
        explicit TBaseStat(size_t linearSize = 1024 * 1024)
            : LinearStat(linearSize, T())
            , MapStat()
            , LinearSize(linearSize)
        {
        }

    public:
        void Clear() {
            LinearStat.clear();
            LinearStat.resize(LinearSize, 0);
            MapStat.clear();
        }

        void GetStat(TVector<std::pair<ui64, T>>& stat) const {
            stat.clear();
            const T empty = T();
            for (size_t i = 0; i < LinearSize; ++i) {
                if (LinearStat[i] != empty) {
                    stat.push_back(std::make_pair((ui64)i, LinearStat[i]));
                }
            }
            for (typename TMap<ui64, T>::const_iterator it = MapStat.begin();
                 it != MapStat.end();
                 ++it) {
                stat.push_back(*it);
            }
        }

    protected:
        T& GetValue(ui64 x) {
            if (Y_LIKELY(x < LinearSize)) {
                return LinearStat[x];
            } else {
                return MapStat[x];
            }
        }

    protected:
        TVector<T> LinearStat;
        TMap<ui64, T> MapStat;
        const size_t LinearSize;
    };

    class TSizeStat: private TBaseStat<ui64> {
    public:
        using TBaseStat::Clear;
        using TBaseStat::GetStat;

        explicit TSizeStat(size_t linearSize = 1024 * 1024)
            : TBaseStat(linearSize)
        {
        }

    public:
        void Inc(ui64 size, ui64 incSize = 1) {
            ui64& value = TBaseStat::GetValue(size);
            value += incSize;
        }
    };

    class TMemStat: private TBaseStat<std::pair<ui64, ui64>> {
    public:
        using TBaseStat::Clear;
        using TBaseStat::GetStat;

        explicit TMemStat(size_t linearSize = 1024 * 1024)
            : TBaseStat(linearSize)
            , BaseRss(0)
            , LastRss(0)
        {
            BaseRss = TRusage::Get().MaxRss;
        }

    public:
        void Add(ui64 size) {
            ui64 rss = TRusage::Get().MaxRss;
            if (rss <= LastRss) {
                return;
            }
            LastRss = rss;

            if (rss >= BaseRss) {
                rss -= BaseRss;
            } else {
                rss = 0;
            }

            std::pair<ui64, ui64>& value = TBaseStat::GetValue(size);
            if (value == std::pair<ui64, ui64>(0, 0)) {
                value = std::make_pair(rss, rss);
            } else {
                value.first = Min(rss, value.first);
                value.second = Max(rss, value.second);
            }
        }

        ui64 GetBaseRss() const {
            return BaseRss;
        }

        ui64 GetLastRss() const {
            return LastRss;
        }

    private:
        ui64 BaseRss;
        ui64 LastRss;
    };

    class TTimeStat: private TBaseStat<std::pair<ui64, ui64>> {
    public:
        using TBaseStat::Clear;
        using TBaseStat::GetStat;

        explicit TTimeStat(size_t linearSize = 1024 * 1024)
            : TBaseStat(linearSize)
        {
        }

    public:
        void Ins(ui64 size, ui64 milliseconds) {
            std::pair<ui64, ui64>& value = TBaseStat::GetValue(size);
            if (value == std::pair<ui64, ui64>(0, 0)) {
                value = std::make_pair(milliseconds, milliseconds);
            } else {
                value.first = Min(milliseconds, value.first);
                value.second = Max(milliseconds, value.second);
            }
        }
    };
}
