#include "utils.h"

#include <library/cpp/auth_client_parser/cookie.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAuthClientParser;

Y_UNIT_TEST_SUITE(ParserTest) {
    Y_UNIT_TEST(common) {
        std::pair<std::string, ui64> session = TUtils::CreateFreshSession(
            ".-23.4.23456764.2:434:244.8:2323:2323.13:12323.23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456764, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);

        session = TUtils::CreateFreshSession(
            ".-23.4.23456764.2:434:244.8:2323:2323.13:12323.yandex_ua:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(noauth) {
        std::pair<std::string, ui64> session = TUtils::CreateFreshNoAuthSession(2 * 60 * 60 + 10);
        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::NoauthValid, (int)parser.Status());
    }

    Y_UNIT_TEST(noauth2) {
        std::pair<std::string, ui64> session = TUtils::CreateFreshNoAuthSession(2 * 60 * 60 - 10);
        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::NoauthValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(brokenCookie) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse("junk"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse("1234567"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse("3:noauth"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse("2:"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse("1"));
    }

    Y_UNIT_TEST(liteUid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.*23456764.2:434:244.8:2323:2323.13:12323.0:23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456764, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(emptySid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.1130000000000383.0.5:23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(emptySid2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.5.23456767..5:23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456767, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(badVersion) {
        TZeroAllocationCookie parser;
        parser.Parse("4:12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)parser.Status());
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ttl, yexception);
        UNIT_ASSERT_EXCEPTION(parser.User().Uid, yexception);
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ts, yexception);

        parser.Parse("s:12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)parser.Status());
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ttl, yexception);
        UNIT_ASSERT_EXCEPTION(parser.User().Uid, yexception);
        UNIT_ASSERT_EXCEPTION(parser.SessionInfo().Ts, yexception);
    }

    Y_UNIT_TEST(badAge) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("12ff323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badDiff) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(
                                     TUtils::CreateFreshSession(
                                         ".-2gg3.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45")
                                         .first));
    }

    Y_UNIT_TEST(badTTL) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSession(
                                                                        "12323.-23.-4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
    }

    Y_UNIT_TEST(badUid) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSession(
                                                                        ".-23.4.23ff456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSession(
                                                                        ".-23.4.-23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
    }

    Y_UNIT_TEST(badSid) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSession(
                                                                        ".-23.4.23456764.2a:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSession(
                                                                        ".-23.4.23456764.2.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse(TUtils::CreateFreshSession(
                                                                        ".-23.4.23456764.2:1234:123.e:12:12.YA_RU:23453.4343.ab23424cde45646ff45")
                                                                        .first));
    }

    Y_UNIT_TEST(expired) {
        TZeroAllocationCookie parser;
        parser.Parse("1257105354.0.3.27222797.2:66373444:0.62001.27999.1c6eb579a688a5f5c10873e481689934");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(27222797, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1257105354, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(expiredTTL0) {
        std::string age = std::to_string(time(nullptr) - 2 * 60 * 60 + 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.0.23456764.2:434:244.8:2323:2323.13:12323.23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL0_failed) {
        ui64 age_orig = time(nullptr) - 2 * 60 * 60 - 10;
        std::string age = std::to_string(age_orig);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.0.23456764.2:434:244.8:2323:2323.13:12323.23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(23456764, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(age_orig, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(expiredTTL1) {
        std::string age = std::to_string(time(nullptr) - 365 * 24 * 60 * 60);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.1.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL2) {
        std::string age = std::to_string(time(nullptr) - 2 * 60 * 60 + 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.1.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL2_failed) {
        std::string age = std::to_string(time(nullptr) - 2 * 60 * 60 - 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.2.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL3) {
        std::string age = std::to_string(time(nullptr) - 14 * 24 * 60 * 60 + 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.3.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL3_failed) {
        std::string age = std::to_string(time(nullptr) - 14 * 24 * 60 * 60 - 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.3.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL4) {
        std::string age = std::to_string(time(nullptr) - 10 * 60 + 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL4_failed) {
        std::string age = std::to_string(time(nullptr) - 4 * 60 * 60 - 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL5) {
        std::string age = std::to_string(time(nullptr) - 90 * 24 * 60 * 60 + 10);
        std::string session_id = age + ".-23.5.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.5.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
    }

    Y_UNIT_TEST(expiredTTL5_failed) {
        std::string age = std::to_string(time(nullptr) - 90 * 24 * 60 * 60 - 10);
        TZeroAllocationCookie parser;
        parser.Parse(age + ".-23.5.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());
    }
}
