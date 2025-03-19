#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <kernel/fast_user_factors/protos/user_sessions_stat.pb.h>

namespace NFastUserFactors {
    enum class EImageCounter {
        IMAGE_REQUEST_COUNT = 0 /* "request_count" */,
        PAGE_COUNT = 1 /* "page_count"*/,
        LONGSEEN_COUNT = 2 /* "longseen_count" */,
        LONGSEEN_FRESH_COUNT = 3 /* "fresh_longseen_count" */,
        PAGE_NO = 4 /* "page_sum" */,
        SHOWS_COUNT = 5 /* "shows_count" */,
        SHOWS_FRESH_COUNT = 6 /* "shows_fresh_count" */,
        CLICKS_COUNT = 7 /* "clicks_count" */,
        CLICKS_FRESH_COUNT = 8 /* "clicks_fresh_count" */,
        RIGHT_CLICKS_COUNT = 9 /* "right_clicks_count" */,
        RIGHT_CLICKS_FRESH_COUNT = 10 /* "fresh_right_clicks_count" */,
        CLICKED_AGE = 11 /* "clicked_age" */,
        CLICKS_COUNT_HOUR_FRESH = 12 /* "hour_fresh_click_count" */,
        GREEN_URL_CLICKS_COUNT = 13 /* "green_url_clicks_count" */,
        GREEN_URL_CLICKS_FRESH_COUNT = 14 /* "fresh_green_url_clicks_count" */,
        QUERYURL_SHOWS_COUNT = 15 /* "queryurl_shows_count" */,
        QUERYURL_CLICK_COUNT = 16 /* "queryurl_clicks_count" */,
        QUERYURL_GREEN_URL_CLICKS_COUNT = 17 /* "queryurl_green_url_clicks_count" */,
        QUERYURL_RIGHT_CLICKS_COUNT = 18 /* "queryurl_right_clicks_count" */,
        QUERYURL_PAGE_NO = 19 /* "queryurl_page_sum" */,
        IMAGES_DESKTOP_REQUEST_COUNT = 20 /* "desktop_request_count" */,
        IMAGES_TOUCH_REQUEST_COUNT = 21 /* "touch_request_count" */,
        FRESH_SURPLUS = 22 /* "fresh_surplus" */,
        COLLECTIONS_SURPLUS = 23 /* "collections_surplus" */,
        RQ_SURPLUS = 24 /* "rq_surplus" */,
    };

    enum class EImageDecay {
        MINUTES_10 = 0 /* "10_min" */,
        MINUTES_30 = 1 /* "30_min" */,
        HOURS_1 = 2 /* "1_hour" */,
        HOURS_8 = 3 /* "8_hours" */,
        WITHOUT_DECAY = 4 /* "" */,
        DAYS_1 = 5 /* "1_day" */,
        DAYS_30 = 6 /* "30_days" */,
    };

    // long decays are absent in IMAGE_DECAY_VALUES
    // because current method (AddImageCounter()) generate all possible counters with decays from IMAGE_DECAY_VALUES
    // it's useless and will be fixed during refactoring
    const THashMap<EImageDecay, float> IMAGE_DECAY_VALUES = {
        {EImageDecay::MINUTES_10, 0.007},
        {EImageDecay::MINUTES_30, 0.02},
        {EImageDecay::HOURS_1, 0.04},
        {EImageDecay::HOURS_8, 0.33},
        {EImageDecay::WITHOUT_DECAY, 0},
    };

    // all counters according rtmapreduce/config/user_tasks/fast_user_factors.py
    enum EImageBlenderCounters {
        REQUEST_COUNT_10_MIN = 0,
        REQUEST_COUNT_30_MIN = 1,
        REQUEST_COUNT_1_HOUR = 2,
        REQUEST_COUNT_8_HOURS = 3,
        PAGE_COUNT_10_MIN = 4,
        PAGE_COUNT_30_MIN = 5,
        PAGE_COUNT_1_HOUR = 6,
        PAGE_COUNT_8_HOURS = 7,
        LONGSEEN_COUNT_10_MIN = 8,
        LONGSEEN_COUNT_30_MIN = 9,
        LONGSEEN_COUNT_1_HOUR = 10,
        LOGNSEEN_COUNT_8_HOURS = 11,
        FRESH_LONGSEEN_COUNT_10_MIN = 12,
        FRESH_LONGSEEN_COUNT_30_MIN = 13,
        FRESH_LONGSEEN_COUNT_1_HOUR = 14,
        FRESH_LONGSEEN_COUNT_8_HOURS = 15,
        PAGE_SUM_10_MIN = 16,
        PAGE_SUM_30_MIN = 17,
        PAGE_SUM_1_HOUR = 18,
        PAGE_SUM_8_HOURS = 19,
        SHOWS_COUNT_10_MIN = 20,
        SHOWS_COUNT_30_MIN = 21,
        SHOWS_COUNT_1_HOUR = 22,
        SHOWS_COUNT_8_HOURS = 23,
        SHOWS_FRESH_COUNT_10_MIN = 24,
        SHOWS_FRESH_COUNT_30_MIN = 25,
        SHOWS_FRESH_COUNT_1_HOUR = 26,
        SHOWS_FRESH_COUNT_8_HOURS = 27,
        CLICKS_COUNT_10_MIN = 28,
        CLICKS_COUNT_30_MIN = 29,
        CLICKS_COUNT_1_HOUR = 30,
        CLICKS_COUNT_8_HOURS = 31,
        CLICKS_FRESH_COUNT_10_MIN = 32,
        CLICKS_FRESH_COUNT_30_MIN = 33,
        CLICKS_FRESH_COUNT_1_HOUR = 34,
        CLICKS_FRESH_COUNT_8_HOUR = 35,
        RIGHT_CLICKS_COUNT_10_MIN = 36,
        RIGHT_CLICKS_COUNT_30_MIN = 37,
        RIGHT_CLICKS_COUNT_1_HOUR = 38,
        RIGHT_CLICKS_COUNT_8_HOURS = 39,
        FRESH_RIGHT_CLICK_COUNT_10_MIN = 40,
        FRESH_RIGHT_CLICK_COUNT_30_MIN = 41,
        FRESH_RIGHT_CLICK_COUNT_1_HOUR = 42,
        FRESH_RIGHT_CLICK_COUNT_8_HOURS = 43,
        CLICKED_AGE_10_MIN = 44,
        CLICKED_AGE_30_MIN = 45,
        CLICKED_AGE_1_HOUR = 46,
        CLICKED_AGE_8_HOURS = 47,
        HOUR_FRESH_CLICKS_10_MIN = 48,
        HOUR_FRESH_CLICKS_30_MIN = 49,
        HOUR_FRESH_CLICKS_1_HOUR = 50,
        HOUR_FRESH_CLICKS_8_HOURS = 51,
        GREEN_URL_CLICKS_10_MIN = 52,
        GREEN_URL_CLICKS_30_MIN = 53,
        GREEN_URL_CLICKS_1_HOUR = 54,
        GREEN_URL_CLICKS_8_HOURS = 55,
        FRESH_GREEN_URL_CLICKS_10_MIN = 56,
        FRESH_GREEN_URL_CLICKS_30_MIN = 57,
        FRESH_GREEN_URL_CLICKS_1_HOUR = 58,
        FRESH_GREEN_URL_CLICKS_8_HOURS = 59,
        DESKTOP_REQUEST_COUNT_1_DAY = 60,
        DESKTOP_REQUEST_COUNT_30_DAYS = 61,
        TOUCH_REQUEST_COUNT_1_DAY = 62,
        TOUCH_REQUEST_COUNT_30_DAYS = 63,
        IMAGES_FRESH_SURPLUS_30_MIN = 64,
        IMAGES_FRESH_SURPLUS_1_DAY = 65,
        COLLECTIONS_SURPLUS_1_DAY = 66,
        RQ_SURPLUS_1_DAY = 67,
    };

    using TImageCounter = std::pair<EImageCounter, EImageDecay>;

    bool TryParseImageCounterName(const TStringBuf& name, std::pair<EImageCounter, EImageDecay>& counter);
    void ParseCountersFromRtmr(const TCountersProto& counters, const ui64& queryTimestamp, THashMap<TImageCounter, float>& counterValues);
    float GetCounterValue(const THashMap<TImageCounter, float>& counterValues, const TImageCounter& counter);
    float GetCounterRatio(const float numerator, const float denominator, const float maxValue = 1.0);
}
