#pragma once

#include <array>

#include <kernel/fast_user_factors/counters/decay_counter.h>

namespace NFastUserFactors {

    // Counters order according to rtmapreduce/config/user_tasks/fast_user_factors.py
    enum EBlenderQueryCounters {
        BQC_SHOWS_PERIOD = 0,
        BQC_SHOWS = 1,
        BQC_SHOWS_MOBILE_PERIOD = 2,
        BQC_SUBQUERIES_FRESH_IP = 3,
        BQC_SUBQUERIES_QDF = 4,
        BQC_SUBQUERIES_COUNT = 5,
        BQC_SUBQUERIES_DESKTOP_COUNT = 6,
        BQC_SUBQUERIES_MOBILE_COUNT = 7,
        BQC_SUBQUERIES_DESKTOP_SURPLUS = 8,
        BQC_SUBQUERIES_MOBILE_SURPLUS = 9,
        BQC_GOOD_FRESH_HOSTS = 10,
        BQC_SHOWS_PERIOD_1D = 11,
        BQC_SHOWS_MOBILE_PERIOD_1D = 12,
        BQC_SHOWS_PERIOD_30D = 13,
        BQC_SHOWS_MOBILE_PERIOD_30D = 14,
        BQC_SUBQUERIES_DESKTOP_COUNT_30D = 15,
        BQC_SUBQUERIES_MOBILE_COUNT_30D = 16,

        BQC_TUTOR_RIVAL_HOSTS_SHOWS_7D = 17,
        BQC_TUTOR_RIVAL_HOSTS_SHOWS_30D = 18,
        BQC_TUTOR_RIVAL_HOSTS_CLICKS_7D = 19,
        BQC_TUTOR_RIVAL_HOSTS_CLICKS_30D = 20,
        BQC_TUTOR_RIVAL_HOSTS_LONG_CLICKS_7D = 21,
        BQC_TUTOR_RIVAL_HOSTS_LONG_CLICKS_30D = 22,
        BQC_TUTOR_RIVAL_HOSTS_SURPLUS_7D = 23,
        BQC_TUTOR_RIVAL_HOSTS_SURPLUS_30D = 24,
    };

    enum EBlenderQueryIntentCounters {
        BQIC_RESERVED = 0,
        BQIC_CLICKS_30MIN = 1,
        BQIC_CLICKS_1HOUR = 2,
        BQIC_SURPLUS = 3,
        BQIC_WINS = 4,
        BQIC_LOSSES = 5
    };

    enum EBlenderQueryIntentLongCounters {
        BQILC_CLICKS_30MIN = 0,
        BQILC_CLICKS_1HOUR = 1,
        BQILC_LONG_30S_CLICKS_30MIN = 2,
        BQILC_LONG_30S_CLICKS_1HOUR = 3,
        BQILC_LONG_60S_CLICKS_30MIN = 4,
        BQILC_LONG_60S_CLICKS_1HOUR = 5,
        BQILC_LONG_120S_CLICKS_30MIN = 6,
        BQILC_LONG_120S_CLICKS_1HOUR = 7,
        BQILC_SHOWS_30MIN = 8,
        BQILC_SHOWS_1HOUR = 9,
        BQILC_CLICK_UNDER_30MIN = 10,
        BQILC_CLICK_UNDER_1HOUR = 11,
        BQILC_SURPLUS = 12,
        BQILC_WINS = 13,
        BQILC_LOSSES = 14
    };

    // Counters order according to rtmapreduce/config/user_tasks/rapid_clicks.py
    enum ERapidCounters {
        // Pers
        RC_WINS_THRESHOLD30_DECAY3 = 0,
        RC_LOSSES_THRESHOLD30_DECAY3 = 1,
        RC_WINS_THRESHOLD30_DECAY30 = 2,
        RC_LOSSES_THRESHOLD30_DECAY30 = 3,
        RC_WINS_THRESHOLD120_DECAY3 = 4,
        RC_LOSSES_THRESHOLD120_DECAY3 = 5,
        RC_WINS_THRESHOLD120_DECAY30 = 6,
        RC_LOSSES_THRESHOLD120_DECAY30 = 7,
        RC_WINS_THRESHOLD5_DECAY30 = 8,
        RC_LOSSES_THRESHOLD5_DECAY30 = 9,
        RC_WINS_LOG5_DECAY30 = 10,
        RC_LOSSES_LOG5_DECAY30 = 11,
        RC_WINS_LOG300_DECAY30 = 12,
        RC_LOSSES_LOG300_DECAY30 = 13,
        RC_WINS_CLICKWISE_LOG60_DECAY30 = 14,
        RC_LOSSES_CLICKWISE_LOG60_DECAY30 = 15,
        RC_CLICKS_THRESHOLD1_DECAY30 = 16,
        RC_CLICKS_THRESHOLD120_DECAY30 = 17,
        RC_CLICKS_THRESHOLD300_DECAY30 = 18,
        RC_SHOWS_DECAY30 = 19,
        // Not pers
        RC_CLICKS_THRESHOLD120_DECAY1 = 20,
        RC_CLICKS_LOG5_DECAY30 = 21,
        RC_SHOWS_DECAY1 = 22,
        RC_CLICKS_DWELLTIME600_DECAY30 = 23,
        RC_CLICKS_DWELLTIME600_DECAY1 = 24,
        // ODD
        RC_CLICKS_ODD01_DECAY30 = 25,
        RC_CLICKS_ODD02_DECAY30 = 26,
        RC_CLICKS_ODD03_DECAY30 = 27,
        RC_CLICKS_ODD04_DECAY30 = 28,
        RC_CLICKS_ODD05_DECAY30 = 29,
        RC_CLICKS_ODD06_DECAY30 = 30,
        RC_CLICKS_ODD07_DECAY30 = 31,
        RC_CLICKS_ODD08_DECAY30 = 32,
        RC_CLICKS_ODD085_DECAY30 = 33,
        RC_CLICKS_ODD09_DECAY30 = 34,

        RC_SHOWS_DECAY3 = 35,

        //Positional shows
        RC_SHOWS_POSITION0_DECAY30 = 36,
        RC_SHOWS_POSITION0_DECAY3 = 37,
        RC_SHOWS_POSITION1_DECAY30 = 38,
        RC_SHOWS_POSITION1_DECAY3 = 39,
        RC_SHOWS_POSITION2_DECAY30 = 40,
        RC_SHOWS_POSITION2_DECAY3 = 41,
        RC_SHOWS_POSITION3_DECAY30 = 42,
        RC_SHOWS_POSITION3_DECAY3 = 43,
        RC_SHOWS_POSITION4_DECAY30 = 44,
        RC_SHOWS_POSITION4_DECAY3 = 45,

        //Positional threshold 30 clicks
        RC_CLICKS_POSITION0_THRESHOLD30_DECAY3 = 46,
        RC_CLICKS_POSITION0_THRESHOLD30_DECAY30 = 47,
        RC_CLICKS_POSITION1_THRESHOLD30_DECAY3 = 48,
        RC_CLICKS_POSITION1_THRESHOLD30_DECAY30 = 49,
        RC_CLICKS_POSITION2_THRESHOLD30_DECAY3 = 50,
        RC_CLICKS_POSITION2_THRESHOLD30_DECAY30 = 51,
        RC_CLICKS_POSITION3_THRESHOLD30_DECAY3 = 52,
        RC_CLICKS_POSITION3_THRESHOLD30_DECAY30 = 53,
        RC_CLICKS_POSITION4_THRESHOLD30_DECAY3 = 54,
        RC_CLICKS_POSITION4_THRESHOLD30_DECAY30 = 55,

        //Positional threshold 120 clicks
        RC_CLICKS_POSITION0_THRESHOLD120_DECAY3 = 56,
        RC_CLICKS_POSITION0_THRESHOLD120_DECAY30 = 57,
        RC_CLICKS_POSITION1_THRESHOLD120_DECAY3 = 58,
        RC_CLICKS_POSITION1_THRESHOLD120_DECAY30 = 59,
        RC_CLICKS_POSITION2_THRESHOLD120_DECAY3 = 60,
        RC_CLICKS_POSITION2_THRESHOLD120_DECAY30 = 61,
        RC_CLICKS_POSITION3_THRESHOLD120_DECAY3 = 62,
        RC_CLICKS_POSITION3_THRESHOLD120_DECAY30 = 63,
        RC_CLICKS_POSITION4_THRESHOLD120_DECAY3 = 64,
        RC_CLICKS_POSITION4_THRESHOLD120_DECAY30 = 65,

        //For freshness (QUERY_DOPP_LITE etc)
        RC_SHOWS_DECAY01 = 66,
        RC_SHOWS_DECAY025 = 67,
        RC_SHOWS_DECAY05 = 68,
        RC_SHOWS_DECAY7 = 69,
        RC_SHOWS_DECAY14 = 70,
        RC_SHOWS_DECAY60 = 71,
        // Note: do not forget to update RAPID_DECAY_COUNTERS below while adding new counters!
    };

    // Decay counters according to ERapidCounters
    static const std::initializer_list<TDecayCounter> RAPID_DECAY_COUNTERS = {
        // Pers
        TDecayCounter(3.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        // Not pers
        TDecayCounter(1.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        // ODD
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),

        TDecayCounter(3.0),

        // positional shows
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),

        // positional threshold(30) clicks
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),

        // positional threshold(120) clicks
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),

        //For freshness (QUERY_DOPP_LITE etc)
        TDecayCounter(0.1),
        TDecayCounter(0.25),
        TDecayCounter(0.5),
        TDecayCounter(7.0),
        TDecayCounter(14.0),
        TDecayCounter(60.0),
    };

    static const std::initializer_list<TDecayCounter> VERSION_ID_500_RAPID_DECAY_COUNTERS = {
        TDecayCounter(1.0),
        TDecayCounter(30.0),
    };

    enum ESpylogRapidCounters {
        SRC_TRAF_THRESHOLD1_DECAY1 = 0,
        SRC_TRAF_THRESHOLD1_DECAY3 = 1,
        SRC_TRAF_THRESHOLD1_DECAY5 = 2,
        SRC_TRAF_THRESHOLD1_DECAY30 = 3,
        SRC_TRAF_THRESHOLD30_DECAY1 = 4,
        SRC_TRAF_THRESHOLD30_DECAY30 = 5,
        SRC_TRAF_THRESHOLD60_DECAY1 = 6,
        SRC_TRAF_THRESHOLD60_DECAY30 = 7,
        SRC_TRAF_THRESHOLD120_DECAY5 = 8,
        SRC_TRAF_THRESHOLD120_DECAY30 = 9,
    };

    static const std::initializer_list<TDecayCounter> SPYLOG_RAPID_DECAY_COUNTERS = {
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(30.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
    };

    enum EWatchlogRapidCounters {
        WRC_TRAF_THRESHOLD1_DECAY1 = 0,
        WRC_TRAF_THRESHOLD1_DECAY3 = 1,
        WRC_TRAF_THRESHOLD1_DECAY5 = 2,
        WRC_TRAF_THRESHOLD1_DECAY30 = 3,
        WRC_TRAF_THRESHOLD30_DECAY1 = 4,
        WRC_TRAF_THRESHOLD30_DECAY3 = 5,
        WRC_TRAF_THRESHOLD30_DECAY5 = 6,
        WRC_TRAF_THRESHOLD30_DECAY30 = 7,
        WRC_TRAF_THRESHOLD60_DECAY1 = 8,
        WRC_TRAF_THRESHOLD60_DECAY3 = 9,
        WRC_TRAF_THRESHOLD60_DECAY5 = 10,
        WRC_TRAF_THRESHOLD60_DECAY30 = 11,
        WRC_TRAF_THRESHOLD120_DECAY1 = 12,
        WRC_TRAF_THRESHOLD120_DECAY3 = 13,
        WRC_TRAF_THRESHOLD120_DECAY5 = 14,
    };

    static const std::initializer_list<TDecayCounter> WATCHLOG_RAPID_DECAY_COUNTERS = {
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
    };

    enum EYabsRapidCounters {
        YRC_CLICKS_THRESHOLD0_DECAY1 = 0,
        YRC_CLICKS_THRESHOLD0_DECAY3 = 1,
        YRC_CLICKS_THRESHOLD0_DECAY5 = 2,
        YRC_CLICKS_THRESHOLD0_DECAY30 = 3,
        YRC_SHOWS_DECAY1 = 4,
        YRC_SHOWS_DECAY3 = 5,
        YRC_SHOWS_DECAY5 = 6,
        YRC_SHOWS_DECAY30 = 7,
    };

    static const std::initializer_list<TDecayCounter> YABS_RAPID_DECAY_COUNTERS = {
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
    };

    enum EZenRapidCounters {
        ZRC_SHOWS_DECAY1 = 0,
        ZRC_SHOWS_DECAY3 = 1,
        ZRC_SHOWS_DECAY5 = 2,
        ZRC_SHOWS_DECAY30 = 3,
        ZRC_CLICKS_THRESHOLD0_DECAY1 = 4,
        ZRC_CLICKS_THRESHOLD0_DECAY3 = 5,
        ZRC_CLICKS_THRESHOLD0_DECAY5 = 6,
        ZRC_CLICKS_THRESHOLD0_DECAY30 = 7,
        ZRC_SHORT_DECAY1 = 8,
        ZRC_SHORT_DECAY3 = 9,
        ZRC_SHORT_DECAY30 = 10,
        ZRC_DISLIKE_DECAY30 = 11,
    };

    static const std::initializer_list<TDecayCounter> ZEN_RAPID_DECAY_COUNTERS = {
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(30.0),
        TDecayCounter(30.0),
    };

    enum ETurboRapidCounters {
        TRC_SHOWS_DECAY1 = 0,
        TRC_SHOWS_DECAY3 = 1,
        TRC_SHOWS_DECAY5 = 2,
        TRC_SHOWS_DECAY30 = 3,
        TRC_CLICKS_THRESHOLD0_DECAY1 = 4,
        TRC_CLICKS_THRESHOLD0_DECAY3 = 5,
        TRC_CLICKS_THRESHOLD0_DECAY5 = 6,
        TRC_CLICKS_THRESHOLD0_DECAY30 = 7,
        TRC_PAGESCROLL_THRESHOLD95_DECAY1 = 8,
    };

    static const std::initializer_list<TDecayCounter> TURBO_RAPID_DECAY_COUNTERS = {
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
        TDecayCounter(3.0),
        TDecayCounter(5.0),
        TDecayCounter(30.0),
        TDecayCounter(1.0),
    };

}
