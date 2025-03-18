#include "utils.h"

#include <library/cpp/auth_client_parser/cookie.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAuthClientParser;

Y_UNIT_TEST_SUITE(ParserTestV3) {
    Y_UNIT_TEST(common) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.0M_i.abQ_s-kSWb0T646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(111111, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(common2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".1.0.1234567890:_wDuAN0AzAC7AKoAAAAAAA:7f.100|111111.1.101.0:12.1:1234|m:YA_RU:23453.-__-.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(111111, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
    }

    Y_UNIT_TEST(common3) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".5.1.1234567890:AQAAfw:7f.100.a:qqq|1234.2.0|1130000000000383.-1.3f2.1:1234.0:12.f:q|m:YA_RU:23453.4343.ab23424cde45646ff45");

        TZeroAllocationCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, parser.User().Uid);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(12, parser.User().Lang);
    }

    Y_UNIT_TEST(badFormat) {
        std::pair<std::string, ui64> session;

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:AQAAfw:7f.100.m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.3.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.a.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badHeader) {
        std::pair<std::string, ui64> session;

        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("3:-5.1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45"));

        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TZeroAllocationCookie().Parse("3:asdf.1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45"));

        session = TUtils::CreateFreshSessionV3(".-1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".s.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.-4.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.s.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890a:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0..100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7qf.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:-7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.1234567890:ASDFGH:7f.-100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.qwe|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.1|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.2f.|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.s:a|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.-2:a|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badFooter) {
        std::pair<std::string, ui64> session;

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.asdf.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU.23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badUser) {
        std::pair<std::string, ui64> session;

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100||m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|asdf.1.102|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|-100.1.100|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|*111111.10.100|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111..100|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.s.1|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1..1:2|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.-5.1:2|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.-1.200.z|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.s:a|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.-3:a|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.abc|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.1:2.2:3.p:q|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL0) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".0.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL0_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".0.0.1234567890:ASDFGH:7f.100.1:a.b:c|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL1) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.a:|111111.1000.202|m:YA_RU:23453.4343.ab23424cde45646ff45", 365 * 24 * 60 * 60);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".2.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234.2:123.f:f|m:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL2_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".2.0.1234567890:ASDFGH:7f.100|111111.-1.202.1:1234|m:12:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL3) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".3.0.1234567890:ASDFGH:7f.100.f:::|111111.1.202.1:1234|1:23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL3_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".3.0.1234567890:ASDFGHjklmnop:7f.0|111111.0.202.f:1234|23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL4) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".4.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 4 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL4_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".4.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 4 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL5) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".5.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 90 * 24 * 60 * 60 - 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)TZeroAllocationCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL5_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".5.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 90 * 24 * 60 * 60 + 10);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)TZeroAllocationCookie().Parse(session.first));
    }
}
