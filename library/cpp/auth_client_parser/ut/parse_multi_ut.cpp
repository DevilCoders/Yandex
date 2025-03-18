#include "utils.h"

#include <library/cpp/auth_client_parser/cookie.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAuthClientParser;

Y_UNIT_TEST_SUITE(ParserTestMulti) {
    Y_UNIT_TEST(common) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.23456764.2:434:244.8:2323:2323.669:1.668:5.13:12323.23453.4343.ab23424cde45646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT(userinfo.IsStaff());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(noauth) {
        std::pair<std::string, ui64> session = TUtils::CreateFreshNoAuthSession(2 * 60 * 60 + 10);
        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::NoauthValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_EXCEPTION(parser.Users(), yexception);

        session = TUtils::CreateFreshNoAuthSession(2 * 60 * 60 - 10);
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::NoauthValid, (int)parser.Status());
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_EXCEPTION(parser.Users(), yexception);
    }

    Y_UNIT_TEST(brokenCookie) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse("junk"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse("1234567"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse("3:noauth"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse("2:"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse("1"));
    }

    Y_UNIT_TEST(liteUid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.*23456764.2:434:244.8:2323:2323.13:12323.0:23453.4343.ab23424cde45646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.IsLite());
        UNIT_ASSERT(!userinfo.IsBetatester());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(emptySid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.23456764.0.5:23453.4343.ab23424cde45646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(emptySid2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.5.23456767..5:23453.4343.ab23424cde45646ff45");

        TFullCookie parser;
        parser.Parse(session.first);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(session.second, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456767, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT(userinfo.HavePwd());
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(badVersion) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("4:12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("s:12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badAge) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("12ff323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45"));
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("-12323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badDiff) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-2gg3.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badTTL) {
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid,
                                 (int)TFullCookie().Parse("12323.-23.-4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45"));
    }

    Y_UNIT_TEST(badUid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.23ff456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badUid2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.-23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badSid) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.23456764.2a:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badSid2) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.23456764.2.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(badSid3) {
        std::pair<std::string, ui64> session =
            TUtils::CreateFreshSession(".-23.4.23456764.2.8.YA_RU:23453.4343.ab23424cde45646ff45");
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::Invalid, (int)TFullCookie().Parse(session.first));
    }

    Y_UNIT_TEST(expiredTTL0) {
        ui64 age_orig = time(nullptr) - 2 * 60 * 60 + 10;
        std::string age = std::to_string(age_orig);
        std::string session_id = age + ".-23.0.23456764.2:434:244.8:2323:2323.13:12323.23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(age_orig, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL0_failed) {
        ui64 age_orig = time(nullptr) - 2 * 60 * 60 - 10;
        std::string age = std::to_string(age_orig);
        std::string session_id = age + ".-23.0.23456764.2:434:244.8:2323:2323.13:12323.23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(age_orig, parser.SessionInfo().Ts);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL1) {
        std::string age = std::to_string(time(nullptr) - 365 * 24 * 60 * 60);
        std::string session_id = age + ".-23.1.1130000000000383.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL2) {
        std::string age = std::to_string(time(nullptr) - 2 * 60 * 60 + 10);
        std::string session_id = age + ".-23.2.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL2_failed) {
        std::string age = std::to_string(time(nullptr) - 2 * 60 * 60 - 10);
        std::string session_id = age + ".-23.2.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(2, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL3) {
        std::string age = std::to_string(time(nullptr) - 14 * 24 * 60 * 60 + 10);
        std::string session_id = age + ".-23.3.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL3_failed) {
        std::string age = std::to_string(time(nullptr) - 14 * 24 * 60 * 60 - 10);
        std::string session_id = age + ".-23.3.1130000000000383.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(3, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(1130000000000383ull, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL4) {
        std::string age = std::to_string(time(nullptr) - 10 * 60 + 10);
        std::string session_id = age + ".-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL4_failed) {
        std::string age = std::to_string(time(nullptr) - 4 * 60 * 60 - 10);
        std::string session_id = age + ".-23.4.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(4, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL5) {
        std::string age = std::to_string(time(nullptr) - 90 * 24 * 60 * 60 + 10);
        std::string session_id = age + ".-23.5.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularMayBeValid, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }

    Y_UNIT_TEST(expiredTTL5_failed) {
        std::string age = std::to_string(time(nullptr) - 90 * 24 * 60 * 60 - 10);
        std::string session_id = age + ".-23.5.23456764.2:434:244.8:2323:2323.13:12323.YA_RU:23453.4343.ab23424cde45646ff45";

        TFullCookie parser;
        parser.Parse(session_id);
        UNIT_ASSERT_VALUES_EQUAL((int)EParseStatus::RegularExpired, (int)parser.Status());

        UNIT_ASSERT_VALUES_EQUAL(1, parser.SessionInfo().Version);
        UNIT_ASSERT_VALUES_EQUAL(5, parser.SessionInfo().Ttl);
        UNIT_ASSERT_VALUES_EQUAL(parser.Users().at(0).Uid, parser.DefaultUser().Uid);
        UNIT_ASSERT_VALUES_EQUAL(1, parser.Users().size());
        UNIT_ASSERT_VALUES_EQUAL("2323:2323", parser.SessionInfo().Authid);
        UNIT_ASSERT_VALUES_EQUAL(0, parser.SessionInfo().Kv.size());

        const TUserInfoExt& userinfo = parser.DefaultUser();
        UNIT_ASSERT_VALUES_EQUAL(23456764, userinfo.Uid);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.PwdCheckDelta);
        UNIT_ASSERT_VALUES_EQUAL(1, userinfo.Lang);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.SocialId);
        UNIT_ASSERT_VALUES_EQUAL(0, userinfo.Kv.size());

        UNIT_ASSERT_VALUES_EQUAL(&userinfo, &parser.Users().at(0));
    }
}
