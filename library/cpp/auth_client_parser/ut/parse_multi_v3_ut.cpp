#include "utils.h"

#include <library/cpp/auth_client_parser/cookie.h>

#include <library/cpp/testing/unittest/registar.h>

#include <string.h>

using namespace NAuthClientParser;

Y_UNIT_TEST_SUITE(ParserTestMultiV3) {
    Y_UNIT_TEST(common) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".0.0.1234567890:ASDFGH:7f.101|111111.1.202.1:1234|m:YA_RU:23453.0M_i.abQ_s-kSWb0T646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Kv.size());
        TKeyValue kv({{1, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);

        session = TUtils::CreateFreshSessionV3(".3.0.1234567890:_wDuAN0AzAC7AKoAAAAAAA:7f.100|111111.1.101.0:12.1:1234|m:YA_RU:23453.-__-.ab23424cde45646ff45");

        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:_wDuAN0AzAC7AKoAAAAAAA:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsLite());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(12, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(2, userinfo.Kv.size());
        kv = TKeyValue({{0, "12"}, {1, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);

        session = TUtils::CreateFreshSessionV3(".5.1.1234567890:AQAAfw:7f.1.3:qqq|111111.-100.3f2.1:1234.0:12|1130000000000383.0.0|m:YA_RU:23453.4343.ab23424cde45646ff45");

        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(1).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT(!parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:AQAAfw:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Kv.size());
        kv = TKeyValue({{3, "qqq"}});
        UNIT_ASSERT_EQUAL(kv, parser.SessionInfo().Kv);

        userinfo = parser.Users().at(0);
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(-100, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(12, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(2, userinfo.Kv.size());
        kv = TKeyValue({{1, "1234"}, {0, "12"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);

        userinfo = parser.Users().at(1);
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());
    }

    Y_UNIT_TEST(badFormat) {
        std::pair<std::string, ui64> session;

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:AQAAfw:7f.100.m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.a.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badHeader) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("3:-5.1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45"));

        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("3:asdf.1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45"));

        std::pair<std::string, ui64> session = TUtils::CreateFreshSessionV3(".-1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".s.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3("..0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.-4.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.s.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.3.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890a:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0..100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7qf.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:-7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.1234567890:ASDFGH:7f.-100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.1234567890:ASDFGH:7f.1000000000|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.qwe|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.1|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.2f.|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.s:a|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.-2:a|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badFooter) {
        std::pair<std::string, ui64> session;

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU:23453.4343.asdf.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.302.1:1234.0:12|m:YA_RU.23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badUser) {
        std::pair<std::string, ui64> session;

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100||m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|asdf.1.102|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|-100.1.100|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|*111111.10.100|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|9999988888777776666655555.1.102|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111..100|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.s.1|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1..1:2|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.-5.1:2|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.-1.200.z|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.s:a|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.-3:a|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.abc|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100|111111.1.200.1:2.2:3.p:q|m:YA_RU:23453.4343.asdfab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL0) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".0.0.1234567890:ASDFGH:7f.3.|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsSuspicious());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Kv.size());
        TKeyValue kv({{1, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }

    Y_UNIT_TEST(expiredTTL0_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".0.0.1234567890:ASDFGH:7f.100.1:a.7:c|111111.1.202.1:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(!parser.SessionInfo().IsSuspicious());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        TKeyValue kv({{1, "a"}, {7, "c"}});
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Kv.size());
        UNIT_ASSERT_EQUAL(kv, parser.SessionInfo().Kv);

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Kv.size());
        kv = TKeyValue({{1, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }

    Y_UNIT_TEST(expiredTTL1) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".1.0.1234567890:ASDFGH:7f.100.789:|111111.1000.1202|m:YA_RU:23453.4343.ab23424cde45646ff45", 365 * 24 * 60 * 60);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        TKeyValue kv({{1929, ""}});
        UNIT_ASSERT_EQUAL(kv, parser.SessionInfo().Kv);

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1000, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT(userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());
    }

    Y_UNIT_TEST(expiredTTL2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".2.0.1234567890:ASDFGH:7f.100|111111.100.c02.1:1234.2:123.f:f|m:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(100, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.IsLite());
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT(userinfo.IsGlogouted());
        UNIT_ASSERT(userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        TKeyValue kv({{1, "1234"}, {2, "123"}, {15, "f"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }

    Y_UNIT_TEST(expiredTTL2_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".2.0.1234567890:ASDFGH:7f.100|111111.-1.1d01.1:1234|m:12:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(-1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.IsLite());
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT(userinfo.IsGlogouted());
        UNIT_ASSERT(userinfo.IsPartnerPddToken());
        UNIT_ASSERT(userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        TKeyValue kv({{1, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }

    Y_UNIT_TEST(expiredTTL3) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".3.0.12345678901234:ASDFGH:7f.100.f:::|111111.1.202|1:23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("12345678901234:ASDFGH:7f", parser.SessionInfo().Authid);
        TKeyValue kv({{15, "::"}});
        UNIT_ASSERT_EQUAL(kv, parser.SessionInfo().Kv);

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());
    }

    Y_UNIT_TEST(expiredTTL3_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".3.0.1234567890:ASDFGHjklmnop:7f.0.|111111.0.201.f:1234|23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(!parser.SessionInfo().IsSuspicious());
        UNIT_ASSERT(!parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGHjklmnop:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.IsLite());
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        TKeyValue kv({{15, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }

    Y_UNIT_TEST(expiredTTL4) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".4.0.1234567890:ASDFGH:7f.100|111111.1.200.1:1234567890|m:YA_RU:23453.4343.ab23424cde45646ff45", 10 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(!parser.SessionInfo().IsSuspicious());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.IsLite());
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234567890, userinfo.SocialId);
        TKeyValue kv({{1, "1234567890"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }

    Y_UNIT_TEST(expiredTTL4_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".4.0.1234567890:ASDFGH:7f.100|111111.1.233.11:1234|m:YA_RU:23453.4343.ab23424cde45646ff45", 4 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(!parser.SessionInfo().IsSuspicious());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.IsLite());
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        TKeyValue kv({{17, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }

    Y_UNIT_TEST(expiredTTL5) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".5.2.1234567890:ASDFGH:7f.100|111111.1.202.1:1234.0:13|1130000000000383.0.0.f:uck|111122223333.44.25|m:YA_RU:23453.4343.ab23424cde45646ff45", 90 * 24 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(2).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(!parser.SessionInfo().IsSuspicious());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.Users().at(0);
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(13, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        TKeyValue kv({{1, "1234"}, {0, "13"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);

        userinfo = parser.Users().at(1);
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        kv = TKeyValue({{15, "uck"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);

        userinfo = parser.Users().at(2);
        UNIT_ASSERT_VALUES_EQUAL(111122223333ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(44, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.IsLite());
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());
    }

    Y_UNIT_TEST(expiredTTL5_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV3(".5.1.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|1130000000000383.-1.2|333332222211111.-10.0.0:0|m:YA_RU:23453.4343.ab23424cde45646ff45", 90 * 24 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(1).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT(!parser.SessionInfo().IsSuspicious());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("1234567890:ASDFGH:7f", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        TUserInfoExt userinfo = parser.Users().at(0);
        UNIT_ASSERT_VALUES_EQUAL(111111, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(1234, userinfo.SocialId);
        TKeyValue kv({{1, "1234"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);

        userinfo = parser.Users().at(1);
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(-1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.IsLite());
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        userinfo = parser.Users().at(2);
        UNIT_ASSERT_VALUES_EQUAL(333332222211111ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(-10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.IsLite());
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT(!userinfo.IsGlogouted());
        UNIT_ASSERT(!userinfo.IsPartnerPddToken());
        UNIT_ASSERT(!userinfo.IsSecure());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        kv = TKeyValue({{0, "0"}});
        UNIT_ASSERT_EQUAL(kv, userinfo.Kv);
    }
}
