#pragma once

#include <kernel/segutils/numerator_utils.h>

#include <library/cpp/deprecated/dater_old/scanner/dater.h>
#include <library/cpp/deprecated/dater_old/dater_stats.h>

#include <kernel/dater/convert_old/old_dater.h>

namespace NSegutils {

class TDaterContext : public TSegmentatorContext {
protected:
    ND2::TDaterScanContext Ctx;
    NDater::TDaterDate BestDate;

    NDater::TDateCoords UrlDates;
    NDater::TDatePositions TitleDates;
    NDater::TDatePositions BodyDates;

    NDater::TDatePositions TopDates;

    NDater::TDaterStats::TDaterStatsCtx StatsCtx;
    NDater::TDaterStats::TDaterStatsCtx FilteredStatsCtx;

    NDater::TDaterStats Stats;
    NDater::TDaterStats FilteredStats;

    NDater::TDatePosition Best;

    bool UseDater2;
    ND2::EDaterMode Mode;

public:

    explicit TDaterContext(TParserContext& ctx)
        : TSegmentatorContext(ctx)
        , Stats(&StatsCtx)
        , FilteredStats(&FilteredStatsCtx)
        , UseDater2()
        , Mode(ND2::DM_MAIN_DATES)
    {}

    void SetUseDater2(bool dater2, ND2::EDaterMode mode = ND2::DM_MAIN_DATES) {
        UseDater2 = dater2;
        Mode = mode;
    }

    void Clear() override;

    NDater::TDatePositions GetAllDates() const;
    NDater::TDatePositions GetTextDates() const;

    const NDater::TDateCoords& GetUrlDates() const {
        return UrlDates;
    }

    const NDater::TDatePositions& GetTitleDates() const {
        return TitleDates;
    }

    const NDater::TDatePositions& GetBodyDates() const {
        return BodyDates;
    }

    const NDater::TDatePosition& GetBestDate() const {
        return Best;
    }

    const NDater::TDatePositions& GetTopDates() const {
        return TopDates;
    }

    const NDater::TDaterStats& GetDaterStats() const {
        return Stats;
    }

    const NDater::TDaterStats& GetDaterStatsFiltered() const {
        return FilteredStats;
    }

    void NumerateDoc();
};

}
