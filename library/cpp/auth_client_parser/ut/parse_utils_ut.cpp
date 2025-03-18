#include <library/cpp/auth_client_parser/cookieutils.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAuthClientParser;

Y_UNIT_TEST_SUITE(ParserUtils) {
    Y_UNIT_TEST(expiresIn) {
        UNIT_ASSERT_VALUES_EQUAL(2 * 60 * 60, NAuthClientParser::NPrivate::ExpiresIn(0).Seconds());
        UNIT_ASSERT_VALUES_EQUAL(90 * 24 * 60 * 60, NAuthClientParser::NPrivate::ExpiresIn(1).Seconds());
        UNIT_ASSERT_VALUES_EQUAL(2 * 60 * 60, NAuthClientParser::NPrivate::ExpiresIn(2).Seconds());
        UNIT_ASSERT_VALUES_EQUAL(14 * 24 * 60 * 60, NAuthClientParser::NPrivate::ExpiresIn(3).Seconds());
        UNIT_ASSERT_VALUES_EQUAL(4 * 60 * 60, NAuthClientParser::NPrivate::ExpiresIn(4).Seconds());
        UNIT_ASSERT_VALUES_EQUAL(90 * 24 * 60 * 60, NAuthClientParser::NPrivate::ExpiresIn(5).Seconds());
        UNIT_ASSERT_VALUES_EQUAL(0, NAuthClientParser::NPrivate::ExpiresIn(6).Seconds());
    }

    Y_UNIT_TEST(parseFooterV1) {
        size_t len = 0;
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseFooterV1("323.13:12323a.23453.4343.ab23424cde45646ff45", len));
        UNIT_ASSERT_VALUES_EQUAL(31, len);
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseFooterV1("323.13:12323.0:23453.4343.ab23424cde45646ff45", len));
        UNIT_ASSERT_VALUES_EQUAL(33, len);
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseFooterV1("323.13:12323.15:0:23453.4343.ab23424cde45646ff45", len));
        UNIT_ASSERT_VALUES_EQUAL(36, len);
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseFooterV1("15:0:23453.4343.ab23424cde45646ff45", len));
        UNIT_ASSERT_VALUES_EQUAL(35, len);

        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseFooterV1("", len));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseFooterV1("15:0:23453|4343.ab23424cde45646ff45", len));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseFooterV1("15:0:23453|4343|ab23424cde45646ff45", len));
    }

    Y_UNIT_TEST(parseFooterV2) {
        bool safe = false;
        bool havePwd = false;
        TLang lang = 0;
        TPwdCheckDelta delta = -17;
        size_t len = 0;

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseFooterV2("2:434:244.8:2323:2323.669:1.0.545...23453.4343.ab23424cde45646ff45",
                                                               safe, lang, havePwd, delta, len));
        UNIT_ASSERT_VALUES_EQUAL(false, safe);
        UNIT_ASSERT_VALUES_EQUAL(545, lang);
        UNIT_ASSERT_VALUES_EQUAL(false, havePwd);
        UNIT_ASSERT_VALUES_EQUAL(-17, delta);
        UNIT_ASSERT_VALUES_EQUAL(39, len);

        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseFooterV2("", safe, lang, havePwd, delta, len));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseFooterV2(".0.545..asd.23453.4343.ab23424cde45646ff45", safe, lang, havePwd, delta, len));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseFooterV2(".0.545.15..23453.4343.ab23424cde45646ff45", safe, lang, havePwd, delta, len));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseFooterV2(".0.-1...23453.4343.ab23424cde45646ff45", safe, lang, havePwd, delta, len));
    }

    Y_UNIT_TEST(parseUid) {
        TString s;
        TUid uid = 0;
        bool isLite = false;

        s = "123";
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseUid(s, uid, isLite));
        UNIT_ASSERT(!isLite);
        UNIT_ASSERT_VALUES_EQUAL(123, uid);

        s = "*123";
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseUid(s, uid, isLite));
        UNIT_ASSERT(isLite);
        UNIT_ASSERT_VALUES_EQUAL(123, uid);

        s = "1234567890123";
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseUid(s, uid, isLite));
        UNIT_ASSERT(!isLite);
        UNIT_ASSERT_VALUES_EQUAL(1234567890123ll, uid);

        s = "*1234567890123";
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseUid(s, uid, isLite));
        UNIT_ASSERT(isLite);
        UNIT_ASSERT_VALUES_EQUAL(1234567890123ll, uid);

        s = "*1234567890123*";
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseUid(s, uid, isLite));

        s = "1234567890123*";
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseUid(s, uid, isLite));

        s = "1234567 890123";
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseUid(s, uid, isLite));
    }

    Y_UNIT_TEST(parseAuthId) {
        TString s;

        s = "1234567890:ASDFGH:7f";
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseAuthId(s));

        s = "1234567890:_wDuAN0AzAC7AKoAAAAAAA:aa";
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseAuthId(s));

        s = "1234567asd890:_wDuAN0AzAC7AKoAAAAAAA:aa";
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseAuthId(s));

        s = "1234567890::aa";
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseAuthId(s));

        s = "1234567890:_wDuAN0AzAC7AKoAAAAAAA:";
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseAuthId(s));

        s = "1234567890:aa";
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseAuthId(s));
    }

    Y_UNIT_TEST(CheckKV) {
        UNIT_ASSERT(NAuthClientParser::NPrivate::CheckKV(""));
        UNIT_ASSERT(NAuthClientParser::NPrivate::CheckKV("123:asd.123:asd"));
        UNIT_ASSERT(NAuthClientParser::NPrivate::CheckKV("123:asd.789:.123:asd"));

        UNIT_ASSERT(!NAuthClientParser::NPrivate::CheckKV("."));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::CheckKV(":"));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::CheckKV("asd:"));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::CheckKV("123:asd."));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::CheckKV("123:asd..123:asd"));
    }

    Y_UNIT_TEST(ParseSids) {
        TSessionFlags sflags = 0;
        TAccountFlags aflags = 0;
        TStringBuf authid;
        unsigned social_id = 0;

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseSids("", sflags, aflags, authid, social_id));
        UNIT_ASSERT_VALUES_EQUAL(0, sflags);
        UNIT_ASSERT_VALUES_EQUAL(0, aflags);
        UNIT_ASSERT(!authid);
        UNIT_ASSERT_VALUES_EQUAL(0, social_id);

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseSids("0", sflags, aflags, authid, social_id));
        UNIT_ASSERT_VALUES_EQUAL(0, sflags);
        UNIT_ASSERT_VALUES_EQUAL(0, aflags);
        UNIT_ASSERT(!authid);
        UNIT_ASSERT_VALUES_EQUAL(0, social_id);

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseSids("123:asd.789:.123:asd", sflags, aflags, authid, social_id));
        UNIT_ASSERT_VALUES_EQUAL(0, sflags);
        UNIT_ASSERT_VALUES_EQUAL(0, aflags);
        UNIT_ASSERT(!authid);
        UNIT_ASSERT_VALUES_EQUAL(0, social_id);

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseSids("1:asd.8:qwerty.58:789.668:2.669:3", sflags, aflags, authid, social_id));
        UNIT_ASSERT_VALUES_EQUAL(0, sflags);
        UNIT_ASSERT_VALUES_EQUAL(0, aflags);
        UNIT_ASSERT_VALUES_EQUAL("qwerty", authid);
        UNIT_ASSERT_VALUES_EQUAL(789, social_id);

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseSids("1:2.8:qwerty.58:789.668:1.669:1", sflags, aflags, authid, social_id));
        UNIT_ASSERT_VALUES_EQUAL(0x100, sflags);
        UNIT_ASSERT_VALUES_EQUAL(0x300, aflags);
        UNIT_ASSERT_VALUES_EQUAL("qwerty", authid);
        UNIT_ASSERT_VALUES_EQUAL(789, social_id);

        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseSids(".", sflags, aflags, authid, social_id));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseSids(":", sflags, aflags, authid, social_id));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseSids("asd:", sflags, aflags, authid, social_id));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseSids("123:asd.", sflags, aflags, authid, social_id));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseSids("123:asd..123:asd", sflags, aflags, authid, social_id));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseSids("123:asd.7a9:.123:asd", sflags, aflags, authid, social_id));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseSids("1:asd.8:qwerty.58:asd.668:2.669:3", sflags, aflags, authid, social_id));
    }

    Y_UNIT_TEST(ParseLang) {
        TLang lang;
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseLang("100 500", lang));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseLang("100a500", lang));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseLang("en500", lang));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseLang("en ", lang));
        UNIT_ASSERT(!NAuthClientParser::NPrivate::ParseLang("ru.", lang));

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseLang("100", lang));
        UNIT_ASSERT_VALUES_EQUAL(100, lang);
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseLang("1", lang));
        UNIT_ASSERT_VALUES_EQUAL(1, lang);
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseLang("", lang));
        UNIT_ASSERT_VALUES_EQUAL(1, lang);

        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseLang("ru", lang));
        UNIT_ASSERT_VALUES_EQUAL(1, lang);
        UNIT_ASSERT(NAuthClientParser::NPrivate::ParseLang("en", lang));
        UNIT_ASSERT_VALUES_EQUAL(3, lang);
    }
}
