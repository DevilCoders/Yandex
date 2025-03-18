#include "utils.h"

#include <library/cpp/auth_client_parser/cookie.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAuthClientParser;

Y_UNIT_TEST_SUITE(ParserTestV2) {
    Y_UNIT_TEST(common) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.en.1.10.my:YA_RU:23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456764, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(common2) {
        std::pair<std::string, ui64> session = TUtils::CreateFreshSessionV2(
            ".1.4.23456764.2:434:244.8:2323:2323.13:12323.0.545.1.10.23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456764, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(545, parser.User().Lang);
    }

    Y_UNIT_TEST(common3) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.yandex_com:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(noauth) {
        std::pair<std::string, ui64> session = TUtils::CreateFreshNoAuthSession(2 * 60 * 60 + 10);
        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::NoauthValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_EXCEPTION(parser.User().Uid, yexception);
    }

    Y_UNIT_TEST(liteUid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.*23456764.2:434:244.8:2323:2323.13:12323.1.en.1.-1.ttt:1:23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456764, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.User().Lang);
    }

    Y_UNIT_TEST(emptySid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.2.23456764.0.1.en.1.10.0:23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456764, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.User().Lang);
    }

    Y_UNIT_TEST(emptySid2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.1130000000000383..1.en.1.10.23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.User().Lang);
    }

    Y_UNIT_TEST(badAge) {
        TZeroAllocationCookie parser;
        parser.Parse("2:12ff323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)parser.Status());
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ttl, yexception);
        UNIT_ASSERT_EXCEPTION(parser.User().Uid, yexception);
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ts, yexception);
        UNIT_ASSERT_EXCEPTION(parser.User().Lang, yexception);
    }

    Y_UNIT_TEST(badAge2) {
        TZeroAllocationCookie parser;
        parser.Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)parser.Status());
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ttl, yexception);
        UNIT_ASSERT_EXCEPTION(parser.User().Uid, yexception);
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ts, yexception);
        UNIT_ASSERT_EXCEPTION(parser.User().Lang, yexception);
    }

    Y_UNIT_TEST(badKey) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:ddd.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.aaa:bbb:ccc.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.::.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badTTL) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("2:12323.-23.-4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:2.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badUid) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSessionV2(
                                                                        ".-23.4.23ff456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSessionV2(
                                                                        ".-23.4.-23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
    }

    Y_UNIT_TEST(badSid) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSessionV2(
                                                                        ".-23.4.23456764.2a:434:244.8:2323:2323.13:12323.1.10.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSessionV2(
                                                                        ".-23.4.23456764.2.8:2323:2323.13:12323.1.10.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSessionV2(
                                                                        ".-23.4.23456764.8.13:12323.e:123.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
    }

    Y_UNIT_TEST(expiredTTL0) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.0.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL0_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.0.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL1) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.1.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 365 * 24 * 60 * 60);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.2.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL2_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.2.23456764.8:1234:12.13:12323.1.0.1.-1.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL3) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.3.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL3_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.3.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL4) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 4 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL4_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 4 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL5) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.5.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 90 * 24 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL5_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.5.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 91 * 24 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }
}
