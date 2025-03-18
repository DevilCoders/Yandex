#include "antirobot_cookie.h"

#include <antirobot/lib/keyring.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <library/cpp/testing/unittest/registar.h>


namespace NAntiRobot {


Y_UNIT_TEST_SUITE(LastVisitsCookie) {
    Y_UNIT_TEST(Serialization) {
        const auto seconds = TInstant::Now().Seconds();

        TLastVisitsCookie lastVisits1(seconds, {
            {TLastVisitsCookie::TRuleId{1}, 6},
            {TLastVisitsCookie::TRuleId{2}, 7},
            {TLastVisitsCookie::TRuleId{3}, 5}
        });

        TString buf;
        NProtoBuf::io::StringOutputStream bufOutput(&buf);
        lastVisits1.Serialize(&bufOutput);

        TLastVisitsCookie lastVisits2(
            {TLastVisitsCookie::TRuleId{1}, TLastVisitsCookie::TRuleId{3}},
            buf
        );

        UNIT_ASSERT_VALUES_EQUAL(lastVisits2.GetLastUpdateSeconds(), seconds);
        UNIT_ASSERT_VALUES_EQUAL(
            lastVisits2.GetIdToLastVisit(),
            (THashMap<TLastVisitsCookie::TRuleId, ui8>{
                {TLastVisitsCookie::TRuleId{1}, 6},
                {TLastVisitsCookie::TRuleId{3}, 5}
            })
        );
    }

    Y_UNIT_TEST(Touch) {
        const auto t1 = TInstant::Now();

        TLastVisitsCookie lastVisits(
            (t1 - ANTIROBOT_COOKIE_EPOCH_START).Seconds(),
            {
                {TLastVisitsCookie::TRuleId{1}, 0},
                {TLastVisitsCookie::TRuleId{2}, 1},
                {TLastVisitsCookie::TRuleId{3}, 254},
                {TLastVisitsCookie::TRuleId{4}, 255},
                {TLastVisitsCookie::TRuleId{5}, 200}
            }
        );

        lastVisits.Touch({TLastVisitsCookie::TRuleId{5}}, t1 + TDuration::Hours(2));

        UNIT_ASSERT_VALUES_EQUAL(
            lastVisits.GetIdToLastVisit(),
            (THashMap<TLastVisitsCookie::TRuleId, ui8>{
                {TLastVisitsCookie::TRuleId{1}, 0},
                {TLastVisitsCookie::TRuleId{2}, 3},
                {TLastVisitsCookie::TRuleId{3}, 255},
                {TLastVisitsCookie::TRuleId{4}, 255},
                {TLastVisitsCookie::TRuleId{5}, 1}
            })
        );
    }
}


Y_UNIT_TEST_SUITE(AntirobotCookie) {
    Y_UNIT_TEST(Serialization) {
        const auto seconds = (TInstant::Now() - ANTIROBOT_COOKIE_EPOCH_START).Seconds();

        TLastVisitsCookie lastVisits(
            seconds,
            {{TLastVisitsCookie::TRuleId{1}, 6}}
        );

        const TAntirobotCookie cookie1(std::move(lastVisits));
        const auto cookieStr = cookie1.Serialize();

        const TAntirobotCookie cookie2({TLastVisitsCookie::TRuleId{1}}, cookieStr);

        UNIT_ASSERT_VALUES_EQUAL(cookie2.LastVisitsCookie.GetLastUpdateSeconds(), seconds);
        UNIT_ASSERT_VALUES_EQUAL(
            cookie2.LastVisitsCookie.GetIdToLastVisit(),
            (THashMap<TLastVisitsCookie::TRuleId, ui8>{{TLastVisitsCookie::TRuleId{1}, 6}})
        );
    }
}


} // namespace NAntiRobot
