#include "utils.h"

#include <library/cpp/auth_client_parser/cookie.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAuthClientParser;

Y_UNIT_TEST_SUITE(ParserTestMultiV2) {
    Y_UNIT_TEST(common) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:244.8:2323:2323.13:12323.669:10.1:2.1.7.1.10.my:YA_RU:23453.4343.ab23424cde45646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT(parser.SessionInfo().IsStress());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(!userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(7, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));

        session = TUtils::CreateFreshSessionV2(".1.2.1130000000000383.2:434:244.8:2323:2323.669:1.0.545...23453.4343.ab23424cde45646ff45");

        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo2 = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo2.Uid);
        UNIT_ASSERT_VALUES_EQUAL(-1, userinfo2.PwdCheckDelta);
        UNIT_ASSERT(!userinfo2.HavePwd());
        UNIT_ASSERT(userinfo2.IsStaff());
        UNIT_ASSERT(!userinfo2.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(545, userinfo2.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo2.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo2.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo2, &parser.Users().at(0));
    }

    Y_UNIT_TEST(badFormat) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:244.8:2323:2323.13:12323.yandex_com:23453.4343.ab23424cde45646ff45");

        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(liteUid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.5.*23456764.2:434:244.8:12341234:ASDFASD:ab.13:12323...1.-1.ttt:1:23453.4343.ab23424cde45646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("12341234:ASDFASD:ab", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(-1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.IsLite());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(emptySid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.0.0.3.0.10.0:23453.4343.ab23424cde45646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(3, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));

        session = TUtils::CreateFreshSessionV2(".-23.5.23456764..1.8.1.10.23453.4343.ab23424cde45646ff45");
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo2 = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo2.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo2.PwdCheckDelta);
        UNIT_ASSERT(userinfo2.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(8, userinfo2.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo2.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo2.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo2, &parser.Users().at(0));
    }

    Y_UNIT_TEST(badAge) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:12ff323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badKey) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:ddd.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.aaa:bbb:ccc.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.::.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badTTL) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:12323.-23.-4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:2.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("2:12323.-23.a.23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:2.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badUid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23ff456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.-23456764.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.1111122222333334444455555.2:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badSid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2a:434:244.8:2323:2323.13:12323.1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.2.8:2323:2323.13:12323.1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.-13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badSafe) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:244.8:2323:2323.13:12323.-1.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:211.8:2323:2323.13:12323.5.1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.13:12323.a.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badLang) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.-1.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.8:2323:2323.13:12323.1.e1n.1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badHavePwd) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.1.-1.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.8:2323:2323.13:12323.1.1.8.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.13:12323.1.0.no.10.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badPwdCheckDelta) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.8:2323:2323.13:12323.1.1.1.ab.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));

        session = TUtils::CreateFreshSessionV2(".-23.4.23456764.13:12323.1.0.1.999998888877777.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL0) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.0.1130000000000383.8:1234:12.13:12323.1:22.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL0_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.0.1130000000000383.8:1234:12.13:12323.1:1.1.0.0.0.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL1) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.1.23456764.8:1234:12.13:12323.0.7.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 365 * 24 * 60 * 60);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(!parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(7, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.2.23456764.8:1234:12.13:12323.1.9.0.1000.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(1000, userinfo.PwdCheckDelta);
        UNIT_ASSERT(!userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(9, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL2_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.2.23456764.8:1234:12.13:12323.1.7.1.-1.YA_RU:23453.4343.ab23424cde45646ff45", 2 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(-1, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(7, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL3) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.3.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL3_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.3.23456764.8:1234:12.13:12323.1..1.10.YA_RU:23453.4343.ab23424cde45646ff45", 14 * 24 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL4) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 10 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL4_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.4.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 4 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL5) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.5.23456764.8:1234:12.13:12323.669:11.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 90 * 24 * 60 * 60 - 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL5_failed) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSessionV2(".-23.5.23456764.8:1234:12.13:12323.1.0.1.10.YA_RU:23453.4343.ab23424cde45646ff45", 91 * 24 * 60 * 60 + 10);

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT(parser.SessionInfo().IsSafe());
        UNIT_ASSERT_VALUES_EQUAL("1234:12", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(10, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }
}
