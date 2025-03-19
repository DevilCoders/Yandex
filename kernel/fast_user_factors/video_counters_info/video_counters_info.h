#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NFastUserFactors {
    enum class EVideoCounter {
        VIDEO_REQUEST_COUNT = 0 /* "video_request_count" */,
        SHOWS_VIDEO = 1/* "shows_video" */,
        SHOWS_VIDEOQUICK = 2 /* "shows_videoquick" */,
        SHOWS_VIDEOULTRA = 3/* "shows_videoultra" */,
        CLICKS_VIDEO = 4 /* "clicks_video" */,
        CLICKS_VIDEOQUICK = 5 /* "clicks_videoquick" */,
        CLICKS_VIDEOULTRA = 6/* "clicks_videoultra" */,
        CTR_VIDEO = 7 /* "ctr_video" */,
        CTR_VIDEOQUICK = 8 /* "ctr_videoquick" */,
        CTR_VIDEOULTRA = 9 /* "ctr_videoultra" */,
        CORR_SHOWS_VIDEO = 10 /* "corr_shows_video" */,
        CORR_SHOWS_VIDEOQUICK = 11 /* "corr_shows_videoquick" */,
        CORR_SHOWS_VIDEOULTRA = 12/* "corr_shows_videoultra" */,
        CORR_CLICKS_VIDEO = 13 /* "corr_clicks_video" */,
        CORR_CLICKS_VIDEOQUICK = 14 /* "corr_clicks_videoquick" */,
        CORR_CLICKS_VIDEOULTRA = 15 /* "corr_clicks_videoultra" */,
        VIDEO_SURPLUS_CLICK_SUM = 16 /* "video_surplus_click_sum" */,
        VIDEO_SURPLUS_CLICK_MEAN = 17 /* "video_surplus_click_mean" */,
        VIDEO_SURPLUS_CLICK_MEDIAN = 18 /* "video_surplus_click_median" */,
        VIDEO_SURPLUS_TRUNC_DEPTH_SUM = 19 /* "video_surplus_trunc_depth_sum" */,
        VIDEO_SURPLUS_TRUNC_DEPTH_MEAN = 20 /* "video_surplus_trunc_depth_mean" */,
        VIDEO_SURPLUS_TRUNC_DEPTH_MEDIAN = 21 /* "video_surplus_trunc_depth_median" */,
        VIDEO_SURPLUS_DWELL_TIME_SUM = 22 /* "video_surplus_dwell_time_sum" */,
        VIDEO_SURPLUS_DWELL_TIME_MEAN = 23 /* "video_surplus_dwell_time_mean" */,
        VIDEO_SURPLUS_DWELL_TIME_MEDIAN = 24 /* "video_surplus_dwell_time_median" */,
        VIDEO_SURPLUS_VIEW_DURATION_SUM = 25 /* "video_surplus_view_duration_sum" */,
        VIDEO_SURPLUS_VIEW_DURATION_MEAN = 26 /* "video_surplus_view_duration_mean" */,
        VIDEO_SURPLUS_VIEW_DURATION_MEDIAN = 27 /* "video_surplus_view_duration_median" */,
        BROWSER_VISITS_FROM_SERP = 28 /* "browser_visits_from_serp" */,
        BROWSER_DIRECT_VISITS = 29 /* "browser_direct_visits" */,
        HEARTBEAT_VIEWTIME_SUM = 30 /* "heartbeat_viewtime_sum" */,
        HEARTBEAT_VIEW_DEPTH_SUM = 31 /* "heartbeat_view_depth_sum" */,
        HEARTBEAT_SHORT_VIEW_COUNT = 32 /* "heartbeat_short_view_count" */,

        HEARTBEAT_DEEP_VIEW_COUNT = 33 /* "heartbeat_deep_view_count" */,
        BROWSER_VIEWTIME_SUM = 34 /* "browser_viewtime_sum" */,
        BROWSER_VIEW_DEPTH_SUM = 35 /* "browser_view_depth_sum" */,
        BROWSER_SHORT_VIEW_COUNT = 36 /* "browser_short_view_count" */,
        BROWSER_DEEP_VIEW_COUNT = 37 /* "browser_deep_view_count" */,

        TVT_VIDEO = 38 /* "tvt_video" */,
        TVT_VIDEOQUICK = 39 /* "tvt_videoquick" */,

        VIEWS_VIDEO = 40 /* "views_video" */,
        VIEWS_VIDEOQUICK = 41 /* "views_videoquick" */,

        SHORT_VIEWS_VIDEO = 42 /* "short_views_video" */,
        SHORT_VIEWS_VIDEOQUICK = 43 /* "short_views_videoquick" */,

        LVT_VIDEO = 44 /* "lvt_video" */,
        LVT_VIDEOQUICK = 45 /* "lvt_videoquick" */,

        QUERYURL_SHOWS = 46 /* "queryurl_shows" */,
        QUERYURL_CLICKS = 47 /* "queryurl_clicks" */,
        QUERYURL_SHOWS_TOUCH = 48 /* "queryurl_shows_touch" */,
        QUERYURL_CLICKS_TOUCH = 49 /* "queryurl_clicks_touch" */,
        QUERYURL_VIEWS_TOUCH = 50 /* "queryurl_views_touch" */,
        QUERYURL_3M_VIEWS_TOUCH = 51 /* "queryurl_3m_views_touch" */,
        QUERYURL_TRUNC_DEPTH_TOUCH = 52 /* "queryurl_trunc_depth_touch" */,
        QUERYURL_VIEWS_WITH_DURATION_TOUCH = 53 /* "queryurl_views_with_duration_touch" */,
        QUERYURL_LVT_TOUCH = 54 /* "queryurl_lvt_touch" */
    };

    enum class EVideoDecay {
        HOURS_1 = 0 /* "1_hour" */,
        HOURS_8 = 1 /* "8_hours" */,
        HOURS_24 = 2 /* "24_hours" */,
        HOURS_48 = 3 /* "48_hours" */,
        HOURS_72 = 4 /* "72_hours" */
    };

    const THashMap<EVideoDecay, float> DECAY_VALUES = {
        {EVideoDecay::HOURS_1, 0.04},
        {EVideoDecay::HOURS_8, 0.33},
        {EVideoDecay::HOURS_24, 1.0},
        {EVideoDecay::HOURS_48, 2.0},
        {EVideoDecay::HOURS_72, 3.0},
    };

    bool TryParseVideoCounterName(const TStringBuf& name, std::pair<EVideoCounter, EVideoDecay>& counter);
    TString UnderscoresToCamelCase(const TString& counter);
} // namespace NFastUserFactors
