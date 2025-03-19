#include "old_dater.h"

namespace ND2 {

void ConvertUrlDates(const TDates& dates, NDater::TDatePositions& res) {
    using NDater::TDaterDate;
    using NDater::TDatePosition;
    for (TDates::const_iterator it = dates.begin(); it != dates.end(); ++it) {
        res.push_back(TDatePosition(
                        TDaterDate(it->Data.Year, it->Data.Month, it->Data.Day,
                                   it->Features.FromHost? TDatePosition::FromHost :
                                       it->Features.FromUrlId ? TDatePosition::FromUrlId :
                                           TDatePosition::FromUrl,
                                   it->Features.Has4DigitsInYear, it->Features.MonthIsWord), *it));
    }
}

void ConvertUrlDates(const TDates& dates, NDater::TDateCoords& res) {
    using NDater::TDaterDate;
    using NDater::TDateCoord;
    for (TDates::const_iterator it = dates.begin(); it != dates.end(); ++it) {
        res.push_back(TDateCoord(
                        TDaterDate(it->Data.Year, it->Data.Month, it->Data.Day,
                                   it->Features.FromHost? TDateCoord::FromHost :
                                       it->Features.FromUrlId ? TDateCoord::FromUrlId :
                                           TDateCoord::FromUrl,
                                   it->Features.Has4DigitsInYear, it->Features.MonthIsWord)));
    }
}

void ConvertTitleDates(const TDates& dates, NDater::TDatePositions& res) {
    using NDater::TDaterDate;
    using NDater::TDatePosition;
    for (TDates::const_iterator it = dates.begin(); it != dates.end(); ++it) {
        res.push_back(TDatePosition(
                        TDaterDate(it->Data.Year, it->Data.Month, it->Data.Day,
                                   TDatePosition::FromTitle,
                                   it->Features.Has4DigitsInYear, it->Features.MonthIsWord), *it));
    }
}

void ConvertTextDates(const TDaterDocumentContext& ctx, const TDates& dates, NDater::TDatePositions& res) {
    using NDater::TDaterDate;
    using NDater::TDatePosition;
    using NSegm::TMainContentSpan;
    using NSegm::TMainContentSpans;
    using NSegm::TSegmentSpan;
    using NSegm::TSegmentSpans;

    for (TDates::const_iterator it = dates.begin(); it != dates.end(); ++it) {
        res.push_back(TDatePosition(
                        TDaterDate(it->Data.Year, it->Data.Month, it->Data.Day,
                                   TDatePosition::FromText,
                                   it->Features.Has4DigitsInYear, it->Features.MonthIsWord), *it));
    }

    NDater::MarkDatePositions(res, NSegm::TSpan(), ctx.MainContentSpans, ctx.SegmentSpans);
}


}
