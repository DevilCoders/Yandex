#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NStat {
    class TDurationHistogram {
    public:
        // Group of adjacent column with same size (precision)
        // Left bound is defined by adjacent neighbouring group's RightBound.
        struct TColumnGroup {
            TDuration RightBound;
            TDuration ColumnWidth;

            bool operator<(const TColumnGroup& g) const;
            size_t ColumnIndex(TDuration value, TDuration leftBound) const;
            size_t ColumnCount(TDuration leftBound) const;
        };

        TDurationHistogram(TVector<TColumnGroup>&& groups);

        void RegisterRequest(TDuration duration) {
            FindColumn(duration) += 1;
        }

        // Print accumulated histogram intervals in stat-handle format,
        // see https://wiki.yandex-team.ru/jandekspoisk/sepe/monitoring/stat-handle/
        // Only non-zero columns are printed.
        NSc::TValue ToJson(TDuration unit) const;

    private:
        TDuration LeftBound(size_t groupIndex) const;
        size_t& FindColumn(TDuration d);

    private:
        TVector<TColumnGroup> Groups;    // groups of columns with same width
        TVector<size_t> GroupStartIndex; // i -> number of columns in all groups before Groups[i]
        TVector<size_t> Columns;         // column heights (histogram values)
    };

}
