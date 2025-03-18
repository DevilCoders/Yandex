#include "histogram.h"
#include <util/generic/algorithm.h>

namespace NStat {
    inline bool TDurationHistogram::TColumnGroup::operator<(const TColumnGroup& g) const {
        return RightBound < g.RightBound;
    }

    // Index of a column within the group to put @value in.
    // @leftBound is RightBound of previous group (or 0).
    inline size_t TDurationHistogram::TColumnGroup::ColumnIndex(TDuration value, TDuration leftBound) const {
        Y_ASSERT(value >= leftBound);
        return (value - leftBound).GetValue() / ColumnWidth.GetValue();
    }

    inline size_t TDurationHistogram::TColumnGroup::ColumnCount(TDuration leftBound) const {
        return ColumnIndex(RightBound, leftBound);
    }

    TDurationHistogram::TDurationHistogram(TVector<TColumnGroup>&& groups)
        : Groups(groups)
    {
        StableSort(Groups.begin(), Groups.end());

        GroupStartIndex.reserve(Groups.size());
        size_t totalColumns = 0;
        TDuration leftBound = TDuration::MicroSeconds(0);
        for (const TColumnGroup& g : Groups) {
            GroupStartIndex.push_back(totalColumns);
            totalColumns += g.ColumnCount(leftBound);
            leftBound = g.RightBound;
        }

        // +1 last (saturation) column is for all durations out of any group
        Columns.resize(totalColumns + 1, 0);
    }

    inline TDuration TDurationHistogram::LeftBound(size_t groupIndex) const {
        return groupIndex > 0 ? Groups[groupIndex - 1].RightBound : TDuration::MicroSeconds(0);
    }

    size_t& TDurationHistogram::FindColumn(TDuration d) {
        TColumnGroup fakeGroup = {d, TDuration()};
        auto it = ::UpperBound(Groups.begin(), Groups.end(), fakeGroup);
        if (it == Groups.end())
            return Columns.back();

        size_t g = it - Groups.begin();
        size_t c = Groups[g].ColumnIndex(d, LeftBound(g));
        return Columns[GroupStartIndex[g] + c];
    }

    static inline double InUnits(TDuration v, TDuration unit) {
        return (double)v.MicroSeconds() / unit.MicroSeconds();
    }

    NSc::TValue TDurationHistogram::ToJson(TDuration unit) const {
        NSc::TValue json;
        json.SetArray();

        TDuration leftBound = TDuration::MicroSeconds(0);
        for (size_t g = 0; g < Groups.size(); ++g) {
            size_t colCount = Groups[g].ColumnCount(leftBound);
            for (size_t i = 0; i < colCount; ++i) {
                size_t value = Columns[GroupStartIndex[g] + i];
                if (value) {
                    NSc::TValue& interval = json.Push().SetArray();
                    // note that in stat-handle format we define each column by its _left_ border
                    interval.Push(InUnits(leftBound + Groups[g].ColumnWidth * i, unit));
                    interval.Push(value);
                }
            }
            leftBound = Groups[g].RightBound;
        }

        // last saturation column
        size_t value = Columns.back();
        if (value) {
            NSc::TValue& interval = json.Push().SetArray();
            interval.Push(InUnits(leftBound, unit));
            interval.Push(value);
        }

        return json;
    }

}
