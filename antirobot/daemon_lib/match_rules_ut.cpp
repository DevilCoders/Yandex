#include "cacher_factors.h"
#include "fullreq_info.h"
#include "match_rule_parser.h"
#include "request_params.h"
#include "rule_set.h"
#include "rule.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <antirobot/lib/keyring.h>
#include <antirobot/lib/spravka_key.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/printf.h>

#include <format>

using namespace NAntiRobot;

using namespace NMatchRequest;

namespace {
    const TString REQUEST =  "GET /Search/blah?text=test&abc=abc HTTP/1.1\r\n"
                            "Host: big.news.yandex.ru\r\n"
                            "User-Agent: Mozilla/5.0 RobotAgent/5\r\n"
                            "Cookie: yandexuid=8388098111348154493\r\n"
                            "Referer: http://yandex.ru/blah-blah\r\n"
                            "Some-Header: some-header-value\r\n"
                            "X-Forwarded-For-Y: 46.44.44.50\r\n"
                            "X-Source-Port-Y: 49644\r\n"
                            "X-Start-Time: 1352889168969432\r\n"
                            "X-Req-Id: 1352889168969432-10545734124049569903\r\n"
                            "\r\n";

    const TString REQUEST_FROM_YANDEX =  "GET /Search/blah?text=test&abc=abc HTTP/1.1\r\n"
                            "Host: big.news.yandex.ru\r\n"
                            "User-Agent: Mozilla/5.0 RobotAgent/5\r\n"
                            "Cookie: yandexuid=8388098111348154493\r\n"
                            "Referer: http://yandex.ru/blah-blah\r\n"
                            "Some-Header: some-header-value\r\n"
                            "X-Forwarded-For-Y: 2a02:6b8:b080:8308::1:10\r\n"
                            "X-Source-Port-Y: 49644\r\n"
                            "X-Start-Time: 1352889168969432\r\n"
                            "X-Req-Id: 1352889168969432-10545734124049569903\r\n"
                            "\r\n";

    const TString REQUEST_DEGR =  "GET /search?text=test&abc=abc HTTP/1.1\r\n"
                            "Host: yandex.ru\r\n"
                            "User-Agent: Mozilla/5.0 RobotAgent/5\r\n"
                            "Cookie: spravka=dD0xMTAwMDAwMDAwO2k9MS4xLjEuMTtEPUYxN0MwQkU0MDAzQzMxRUNBQzhCNjgyRERGRjc1ODVFRDlBNkQ3MTEwRTQ4MTQ3NDlFM0UzNTc3RUJBMjNBMEQ7dT0xMjMxMjI0NTM3ODtoPThlNGQxYjQxMmE4ZDUyMGQxZGM2Mjc1ZTVhMjc4YjE0\r\n"
                            "Referer: http://yandex.ru/blah-blah\r\n"
                            "Some-Header: some-header-value\r\n"
                            "X-Forwarded-For-Y: 46.44.44.50\r\n"
                            "X-Source-Port-Y: 49644\r\n"
                            "X-Start-Time: 1352889168969432\r\n"
                            "X-Req-Id: 1352889168969432-10545734124049569903\r\n"
                            "\r\n";

    const TString REQUEST_MULTI =  "GET /Search/blah?text=test&abc=abc HTTP/1.1\r\n"
                            "Host: big.news.yandex.ru\r\n"
                            "User-Agent: Mozilla/5.0 RobotAgent/5\r\n"
                            "Accept-Language: en-US,en;q=0.9,he-IL;q=0.8,he;q=0.7,tr;q=0.6\r\n"
                            "Accept-Language: en-US,en;q=0.9\r\n"
                            "accept-language: en-US,en;q=0.9,he-IL;q=0.8,he;q=0.7,tr;q=0.6\r\n"
                            "accept-language: en-US,en;q=0.9\r\n"
                            "accept-language: en-US\r\n"
                            "Cookie: yandexuid=8388098111348154493\r\n"
                            "Referer: http://yandex.ru/blah-blah\r\n"
                            "Some-Header: some-header-value\r\n"
                            "X-Forwarded-For-Y: 46.44.44.50\r\n"
                            "X-Source-Port-Y: 49644\r\n"
                            "X-Start-Time: 1352889168969432\r\n"
                            "X-Req-Id: 1352889168969432-10545734124049569903\r\n"
                            "\r\n";


    const TString GEOBASE_BIN_PATH = "./geodata6-xurma.bin";

    bool ValidateCurrentTimestamp(const TRule& rule, unsigned timestamp) {
        return AllOf(
            rule.CurrentTimestamp,
            [timestamp] (const auto& cmp) { return cmp.Compare(timestamp); }
        );
    }
}

Y_UNIT_TEST_SUITE_IMPL(TTestMatchRules, TTestAntirobotMediumBase) {
    Y_UNIT_TEST(EmptyInput) {
        UNIT_ASSERT_NO_EXCEPTION(ParseRule(""));
    }

    Y_UNIT_TEST(SimplestInput) {
        const char* remText = "Empty rule";
        ParseRule(Sprintf("rem='%s'", remText));
    }

    Y_UNIT_TEST(DelimiterInput) {
        const char* remText = "Empty rule";
        TRule rule = ParseRule(Sprintf("rem='%s';enabled=yes", remText));
        UNIT_ASSERT_VALUES_EQUAL(rule.Enabled, true);
    }

    Y_UNIT_TEST(Delimiters) {
        UNIT_ASSERT_NO_EXCEPTION(ParseRule(R"(enabled=yes;)"));
        UNIT_ASSERT_NO_EXCEPTION(ParseRule(R"(rem='abc';enabled=yes)"));
        UNIT_ASSERT_NO_EXCEPTION(ParseRule(R"(rem='abc'; enabled=yes)"));
        UNIT_ASSERT_NO_EXCEPTION(ParseRule(R"(rem='abc';enabled=yes; nonblock=no;)"));
        UNIT_ASSERT_NO_EXCEPTION(ParseRule(R"(rem='abc';enabled=yes; nonblock=no; )"));
    }

    Y_UNIT_TEST(BoolValue) {
        {
            TRule rule = ParseRule(R"(enabled=no; nonblock=yes)");
            UNIT_ASSERT_VALUES_EQUAL(rule.Enabled, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.Nonblock, true);
        }
        {
            TRule rule = ParseRule(R"(enabled=yes; nonblock=no)");
            UNIT_ASSERT_VALUES_EQUAL(rule.Enabled, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.Nonblock, false);
        }
        {
            TRule rule = ParseRule(R"(is_tor=yes; is_proxy=no; is_vpn=no; is_hosting=no; is_mobile=no)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IsTor, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsProxy, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsVpn, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsHosting, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsMobile, false);
        }
        {
            TRule rule = ParseRule(R"(is_tor=no; is_proxy=yes; is_vpn=no; is_hosting=no; is_mobile=no)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IsTor, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsProxy, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsVpn, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsHosting, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsMobile, false);
        }
        {
            TRule rule = ParseRule(R"(is_tor=no; is_proxy=no; is_vpn=yes; is_hosting=no; is_mobile=no)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IsTor, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsProxy, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsVpn, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsHosting, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsMobile, false);
        }
        {
            TRule rule = ParseRule(R"(is_tor=no; is_proxy=no; is_vpn=no; is_hosting=yes; is_mobile=no)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IsTor, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsProxy, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsVpn, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsHosting, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsMobile, false);
        }
        {
            TRule rule = ParseRule(R"(is_tor=no; is_proxy=no; is_vpn=no; is_hosting=no; is_mobile=yes)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IsTor, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsProxy, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsVpn, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsHosting, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.IsMobile, true);
        }
        {
            TRule rule = ParseRule(R"(is_whitelist=yes;)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IsWhitelist, true);
        }
        {
            TRule rule = ParseRule(R"(is_whitelist=no;)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IsWhitelist, false);
        }
        {
            UNIT_ASSERT_EXCEPTION(ParseRule(R"(enabled=ye)"), TParseRuleError);
        }
    }

    Y_UNIT_TEST(RegexValue) {
        {
            TRule rule = ParseRule(R"(cgi!=/.*.ru/; doc= /\/search.*/i)");
            UNIT_ASSERT(!rule.CgiString.empty());
            UNIT_ASSERT(!rule.Doc.empty());

            const auto& cgiStringEntry = std::get<TRegexMatcherEntry>(rule.CgiString.back().Value);
            UNIT_ASSERT_VALUES_EQUAL(cgiStringEntry.Expr, R"(.*.ru)");
            UNIT_ASSERT_VALUES_EQUAL(cgiStringEntry.CaseSensitive, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.CgiString.back().ShouldMatch, false);

            const auto& docEntry = std::get<TRegexMatcherEntry>(rule.Doc.back().Value);
            UNIT_ASSERT_VALUES_EQUAL(docEntry.Expr, R"(/search.*)");
            UNIT_ASSERT_VALUES_EQUAL(docEntry.CaseSensitive, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.Doc.back().ShouldMatch, true);
        }
        {
            TRule rule = ParseRule(R"(cgi=/.*.ru/i; doc != /\/search.*/; arrival_time=/.*0/; service_type=/click/)");
            UNIT_ASSERT(rule.CgiString);
            UNIT_ASSERT(rule.Doc);
            UNIT_ASSERT(rule.ArrivalTime);
            UNIT_ASSERT(rule.ServiceType);

            const auto& cgiStringEntry = std::get<TRegexMatcherEntry>(rule.CgiString.back().Value);
            UNIT_ASSERT_VALUES_EQUAL(cgiStringEntry.Expr, R"(.*.ru)");
            UNIT_ASSERT_VALUES_EQUAL(cgiStringEntry.CaseSensitive, false);
            UNIT_ASSERT_VALUES_EQUAL(rule.CgiString.back().ShouldMatch, true);

            const auto& docEntry = std::get<TRegexMatcherEntry>(rule.Doc.back().Value);
            UNIT_ASSERT_VALUES_EQUAL(docEntry.Expr, R"(/search.*)");
            UNIT_ASSERT_VALUES_EQUAL(docEntry.CaseSensitive, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.Doc.back().ShouldMatch, false);

            const auto& arrivalTimeEntry = std::get<TRegexMatcherEntry>(rule.ArrivalTime.back().Value);
            UNIT_ASSERT_VALUES_EQUAL(arrivalTimeEntry.Expr, R"(.*0)");
            UNIT_ASSERT_VALUES_EQUAL(arrivalTimeEntry.CaseSensitive, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.ArrivalTime.back().ShouldMatch, true);

            const auto& serviceTypeEntry = std::get<TRegexMatcherEntry>(rule.ServiceType.back().Value);
            UNIT_ASSERT_VALUES_EQUAL(serviceTypeEntry.Expr, R"(click)");
            UNIT_ASSERT_VALUES_EQUAL(serviceTypeEntry.CaseSensitive, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.ServiceType.back().ShouldMatch, true);
        }
        {
            TRule rule = ParseRule(R"(request=/.*X-Antirobot-Service-Y:.*X-Forwarded-For-Y:.*X-Yandex-Ja3:.*/)");
            UNIT_ASSERT(rule.Request);

            const auto& requestEntry = std::get<TRegexMatcherEntry>(rule.Request.back().Value);
            UNIT_ASSERT_VALUES_EQUAL(requestEntry.Expr, R"(.*X-Antirobot-Service-Y:.*X-Forwarded-For-Y:.*X-Yandex-Ja3:.*)");
            UNIT_ASSERT_VALUES_EQUAL(requestEntry.CaseSensitive, true);
            UNIT_ASSERT_VALUES_EQUAL(rule.Request.back().ShouldMatch, true);
        }
        {
            TRule rule = ParseRule(R"(hodor=/.*be.*/; hodor_hash=/./)");
            UNIT_ASSERT(rule.Hodor);
            UNIT_ASSERT(rule.HodorHash);
        }
    }

    Y_UNIT_TEST(IpValue) {
        {
            TRule rule = ParseRule(R"(ip=1.2.3.4)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IpInterval.IpBeg, TAddr(R"(1.2.3.4)"));
        }
        {
            TRule rule = ParseRule(R"(ip=f1f9::1/16;doc=/.*\.host/)");
            UNIT_ASSERT_VALUES_EQUAL(rule.IpInterval.Print(), TIpInterval::Parse(R"(F1F9::1/16)").Print());
            UNIT_ASSERT_VALUES_EQUAL(std::get<TRegexMatcherEntry>(rule.Doc.back().Value).Expr, R"(.*\.host)");
            UNIT_ASSERT_VALUES_EQUAL(rule.Doc.back().ShouldMatch, true);
        }
    }

    Y_UNIT_TEST(IpFromValue) {
        {
            TRule rule = ParseRule(R"(ip_from=124)");
            UNIT_ASSERT_VALUES_EQUAL(*rule.CbbGroup, TCbbGroupId{124});
        }
        {
            UNIT_ASSERT_EXCEPTION(ParseRule("ip_from=1.2"), TParseRuleError);
        }
    }

    Y_UNIT_TEST(HeaderValue) {
        {
            TRule rule = ParseRule(R"(header['Some-Header']=/val/)");
            THashMap<TString, TVector<TRegexCondition>> expectedHeaders = {
                {"some-header", {TRegexCondition("val", true, true)}}
            };
            UNIT_ASSERT_EQUAL(rule.Headers, expectedHeaders);
        }
        {
            TRule rule = ParseRule(R"(header['Some-Header']!=/val/; header['Another-Header']=/another-val/)");

            THashMap<TString, TVector<TRegexCondition>> expectedHeaders = {
                {"some-header", {TRegexCondition("val", true, false)}},
                {"another-header", {TRegexCondition("another-val", true, true)}}
            };

            UNIT_ASSERT_EQUAL(rule.Headers, expectedHeaders);
        }
    }

    Y_UNIT_TEST(HasHeaderValue) {
        {
            TRule rule = ParseRule(R"(has_header['some-header'] = yes)");
            TVector<THasHeaderCondition> expectedHasHeaders = {
                {"some-header", true}
            };
            UNIT_ASSERT_EQUAL(rule.HasHeaders, expectedHasHeaders);
        }
        {
            TRule rule = ParseRule(R"(has_header['some-header'] = yes; has_header['another-header'] = no)");

            TVector<THasHeaderCondition> expectedHasHeaders = {
                {"some-header", true},
                {"another-header", false}
            };

            UNIT_ASSERT_EQUAL(rule.HasHeaders, expectedHasHeaders);
        }
    }

    Y_UNIT_TEST(NumHeaderValue) {
        {
            TRule rule = ParseRule(R"(num_header['some-header'] = 2)");
            TVector<TNumHeaderCondition> expectedNumHeaders = {
                {"some-header", 2}
            };
            UNIT_ASSERT_EQUAL(rule.NumHeaders, expectedNumHeaders);
        }
        {
            TRule rule = ParseRule(R"(num_header['some-header'] = 2; num_header['another-header'] = 3)");

            TVector<TNumHeaderCondition> expectedNumHeaders = {
                {"some-header", 2},
                {"another-header", 3}
            };

            UNIT_ASSERT_EQUAL(rule.NumHeaders, expectedNumHeaders);
        }
    }

    Y_UNIT_TEST(FactorValue) {
        {
            TRule rule = ParseRule("factor['abcdef']<=42");
            TVector<TFactorCondition> expectedFactors = {
                {"abcdef", {EComparatorOp::LessEqual, 42}}
            };
            UNIT_ASSERT_EQUAL(rule.Factors, expectedFactors);
        }
    }

    Y_UNIT_TEST(CookieAgeValue) {
        {
            TRule rule = ParseRule(R"(cookie_age == 2)");
            UNIT_ASSERT(rule.CookieAge.Compare(2.0));
            UNIT_ASSERT(!rule.CookieAge.Compare(2.5));
        }
        {
            TRule rule = ParseRule(R"(cookie_age > 2.1)");
            UNIT_ASSERT(rule.CookieAge.Compare(2.5));
            UNIT_ASSERT(!rule.CookieAge.Compare(2.1));
        }
        {
            TRule rule = ParseRule(R"(cookie_age >= +2.12)");
            UNIT_ASSERT(rule.CookieAge.Compare(2.12));
            UNIT_ASSERT(!rule.CookieAge.Compare(2));
        }
        {
            TRule rule = ParseRule(R"(cookie_age < -2.12)");
            UNIT_ASSERT(rule.CookieAge.Compare(-2.13));
            UNIT_ASSERT(!rule.CookieAge.Compare(-2.12));
        }
        {
            TRule rule = ParseRule(R"(cookie_age <= -0.12)");
            UNIT_ASSERT(rule.CookieAge.Compare(-0.12));
            UNIT_ASSERT(!rule.CookieAge.Compare(-0.1));
        }
        {
            TRule rule = ParseRule(R"(cookie_age != -0.12)");
            UNIT_ASSERT(rule.CookieAge.Compare(-0.1));
            UNIT_ASSERT(!rule.CookieAge.Compare(-0.12));
        }
    }

    Y_UNIT_TEST(CurrentTimestampValue) {
        {
            TRule rule = ParseRule(R"(current_timestamp == 3600)");
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 3600));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 200));
        }
        {
            TRule rule = ParseRule(R"(current_timestamp > 3600)");
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 3602));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 100));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 3600));
        }
        {
            TRule rule = ParseRule(R"(current_timestamp >= 3600)");
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 3600));
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 3602));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 200));
        }
        {
            TRule rule = ParseRule(R"(current_timestamp < 3600)");
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 200));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 3600));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 3602));
        }
        {
            TRule rule = ParseRule(R"(current_timestamp <= 3600)");
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 3600));
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 200));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 3602));
        }
        {
            TRule rule = ParseRule(R"(current_timestamp != 3600)");
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 3602));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 3600));
        }
        {
            TRule rule = ParseRule(R"(current_timestamp > 3600; current_timestamp < 7200)");
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 3602));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 200));
            UNIT_ASSERT(ValidateCurrentTimestamp(rule, 7000));
            UNIT_ASSERT(!ValidateCurrentTimestamp(rule, 7500));
        }
    }

    Y_UNIT_TEST(EmptyRule) {
        auto textRule = R"(rem="";)";
        TRule rule = ParseRule(textRule);
        UNIT_ASSERT(rule.Doc.empty());
        UNIT_ASSERT(rule.CgiString.empty());
        UNIT_ASSERT(rule.IdentType.empty());
        UNIT_ASSERT(rule.Headers.empty());
        UNIT_ASSERT(rule.NumHeaders.empty());
    }

    Y_UNIT_TEST(RegexMatchLogic) {
        const bool caseYes = true;
        const bool equalYes = true;
        const bool equalNo = false;
        {
            const TPreparedRegexCondition condition({TRegexCondition("A", caseYes, equalYes)});
            const auto& scanner = condition.Scanner;

            UNIT_ASSERT(scanner.Matches("A"));
            UNIT_ASSERT(!scanner.Matches("a"));
            UNIT_ASSERT(!scanner.Matches("^A$")); // No need to use '^' and '$' becouse we use match(), not search()
            UNIT_ASSERT(!scanner.Matches("B"));
            UNIT_ASSERT(!scanner.Matches(""));
        }
        {
            const TPreparedRegexCondition condition({TRegexCondition("/A", caseYes, equalYes)});
            const auto& scanner = condition.Scanner;

            UNIT_ASSERT(scanner.Matches("/A"));
            UNIT_ASSERT(!scanner.Matches("B/A"));
            UNIT_ASSERT(!scanner.Matches("/AB"));
        }

        {
            /* empty regex doesn't match anything */
            /* val = // */
            const TPreparedRegexCondition condition({TRegexCondition("", caseYes, equalYes)});
            const auto& scanner = condition.Scanner;

            UNIT_ASSERT(scanner.Matches(""));
            UNIT_ASSERT(!scanner.Matches("a"));
        }

        {
            /* val != // */
            const TPreparedRegexCondition condition({TRegexCondition("", caseYes, equalNo)});
            const auto& scanner = condition.Scanner;

            UNIT_ASSERT(!scanner.Matches(""));
            UNIT_ASSERT(scanner.Matches("a"));
        }
    }

    void AssertMatched(
        const TRequest& req,
        const TString& ruleText,
        const TRawCacherFactors* cacherFactors = nullptr,
        bool shouldMatch = true
    ) {
        TRuleSet blocker({{TCbbGroupId{0}, {TPreparedRule::Parse(ruleText)}}});
        const auto matched = !blocker.Match(req, cacherFactors).empty();
        UNIT_ASSERT_VALUES_EQUAL_C(matched, shouldMatch, ruleText);
    }

    void AssertNotMatched(
        const TRequest& req,
        const TString& ruleText,
        const TRawCacherFactors* cacherFactors = nullptr
    ) {
        AssertMatched(req, ruleText, cacherFactors, false);
    }

    Y_UNIT_TEST(TestRuleBlocker) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST);

        AssertMatched(*rp, R"(header['host']=/.*\.news\.yandex\.ru/)");
        AssertMatched(*rp, R"(header['host']=/.*\.News\.yandex\.ru/i)");
        AssertMatched(*rp, R"(header['User-Agent']=/.*robotagent.*/i)");
        AssertMatched(*rp, R"(doc=/\/Search.+/)");
        AssertMatched(*rp, R"(doc=/\/search\/blah/i)");
        AssertMatched(*rp, R"(header['User-Agent']=/.*RobotAgent.+/; doc=/\/Search\/.+/)");
        AssertMatched(*rp, R"(header['host']=/.*\.news\.yandex\.ru/; header['User-Agent']=/.*robotagent.*/i; doc=/\/Search\/.+/)");
        AssertMatched(*rp, R"(header['host']=/.*\.news\.yandex\.ru/; header['User-Agent']=/.*RobotAgent.*/; doc=/\/Search\/.+/; cgi=/.*abc.*/)");
        AssertMatched(*rp, R"(ip=46.44.44.50)");
        AssertMatched(*rp, R"(header['referer']=/.+blah.+/)");
        AssertMatched(*rp, R"(header['referer']=/.+blah.+/; header['some-header']=/some-header-value/)");
        AssertMatched(*rp, R"(ident_type=/1-774646834/)");
        AssertMatched(*rp, R"(ident_type=/1-.*/)");
        AssertMatched(*rp, R"(arrival_time=/.*2/)");
        AssertMatched(*rp, R"(service_type=/web/)");

        AssertMatched(*rp, R"(has_header['COOKIE'] = yes)");
        AssertNotMatched(*rp, R"(has_header['COOKIE'] = no)");
        AssertMatched(*rp, R"(has_header['NON-EXIST-HEADER'] = no)");
        AssertMatched(*rp, R"(has_header['COOKIE'] = yes; has_header['NON-EXIST-HEADER'] = no)");
        AssertNotMatched(*rp, R"(has_header['COOKIE'] = yes; has_header['NON-EXIST-HEADER'] = yes)");

        AssertNotMatched(*rp, R"(header['User-Agent']=/.*robotAgent.+/; doc=/\/search\/.+/)");
        AssertNotMatched(*rp, R"(header['host']=/.*\.news\.yandex\.ru/; header['User-Agent']=/.*BAD_USERAGENT.*/; doc=/\/Search\/.+/)");
        AssertNotMatched(*rp, R"(ip=46.44.44.51)");
        AssertNotMatched(*rp, R"(header['Referer']=/.+blah1.+/)");
        AssertNotMatched(*rp, R"(ip=46.44.44.50; header['some-header']=/q/)");
        AssertNotMatched(*rp, R"(ident_type=/2-774646834/)");

        /* Tests an empty rule doesn't block */
        AssertNotMatched(*rp, R"(rem='test')");
        AssertNotMatched(*rp, R"(doc=//)");
        AssertNotMatched(*rp, R"(cgi=//)");
        AssertNotMatched(*rp, R"(header['User-Agent']=//)");
        AssertNotMatched(*rp, R"(header['User-Agent']=//;header['Referer']=//)");
        AssertNotMatched(*rp, R"(doc=//;header['Referer']=//;header['User-Agent']=/NON-MATCHED/)");
        AssertNotMatched(*rp, R"(header['User-Agent']=//;header['Some-Header']=/some-header-value/)");
        AssertNotMatched(*rp, R"(ident_type=//)");

        AssertMatched(*rp, R"(doc!=//)");

        AssertMatched(*rp, R"(nonblock=yes;header['User-Agent']!=//)");
    }

    Y_UNIT_TEST(TestRuleBlockerFromYandex) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST_FROM_YANDEX, {}, {}, GEOBASE_BIN_PATH);
        AssertMatched(*rp, R"(is_whitelist=yes)");
        AssertNotMatched(*rp, R"(is_whitelist=no)");
    }

    Y_UNIT_TEST(TestRuleBlockerNotFromYandex) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST, {}, {}, GEOBASE_BIN_PATH);
        AssertNotMatched(*rp, R"(is_whitelist=yes)");
        AssertMatched(*rp, R"(is_whitelist=no)");
    }

    Y_UNIT_TEST(TestRuleBlockerCountryId) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST, {}, {}, GEOBASE_BIN_PATH);
        AssertMatched(*rp, R"(country_id=/225/)");
        AssertNotMatched(*rp, R"(country_id!=/225/)");
        AssertNotMatched(*rp, R"(country_id=/21/)");
        AssertMatched(*rp, R"(country_id!=/21/)");

        /* glued country */
        AssertMatched(*rp, R"(country_id=/225/; country_id!=/21/)");
        AssertNotMatched(*rp, R"(country_id=/21/; country_id=/225/)");
    }

    Y_UNIT_TEST(TestMulti) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST_MULTI);

        AssertMatched(*rp, R"(num_header['Accept-Language']=5)");
        AssertNotMatched(*rp, R"(num_header['Accept-Language']=3)");
        AssertNotMatched(*rp, R"(num_header['Accept-Language']=1)");
    }

    Y_UNIT_TEST(TestCSHeader) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST_MULTI);

        AssertMatched(*rp, R"(header['host']=/.*\.news\.yandex\.ru/)");
        AssertNotMatched(*rp, R"(csheader['host']=/.*\.news\.yandex\.ru/)");

        AssertMatched(*rp, R"(has_header['host']=yes)");
        AssertNotMatched(*rp, R"(has_csheader['host']=yes)");
        AssertMatched(*rp, R"(has_csheader['Host']=yes)");
        AssertMatched(*rp, R"(has_csheader['host']=no)");

        AssertMatched(*rp, R"(num_header['Accept-Language']=5)");
        AssertMatched(*rp, R"(num_csheader['Accept-Language']=2)");
        AssertMatched(*rp, R"(num_csheader['accept-language']=3)");
    }

    Y_UNIT_TEST(TestGlued) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST);

        /* glued cgi */
        AssertMatched(*rp, R"(cgi=/.*abc.*/; cgi=/.*text.*/)");
        AssertMatched(*rp, R"(cgi=/.*text.*/; cgi=/.*abc.*/)");
        AssertNotMatched(*rp, R"(cgi=/.*abc.*/; cgi=/.*cat.*/)");
        AssertMatched(*rp, R"(cgi=/.*abc.*/; cgi!=/.*cat.*/)");
        AssertMatched(*rp, R"(cgi!=/.*aaa.*/; cgi!=/.*bbb.*/; cgi!=/.*ccc.*/)");
        AssertMatched(*rp, R"(cgi=/.*abc.*/; cgi!=/.*aaa.*/; cgi=/.*test.*/; cgi=/.*text.*/; cgi!=/.*bbb.*/)");
        AssertNotMatched(*rp, R"(cgi=/.*abc.*/; cgi!=/.*text.*/; cgi!=/.*cat.*/)");
        AssertNotMatched(*rp, R"(cgi=/.*abc.*/; cgi!=/.*abc.*/)");
        AssertNotMatched(*rp, R"(cgi!=/.*abc.*/; cgi=/.*text.*/)");

        /* glued headers */
        AssertMatched(*rp, R"(header['host']=/.*news.*/; header['host']=/.*big.*/)");
        AssertNotMatched(*rp, R"(header['host']!=/.*big.*/; header['host']=/.*news.*/)");
        AssertMatched(*rp, R"(header['host']=/.*\.news\.yandex\.ru/; header['host']=/.*news.*/; header['host']=/.*big.*/;)");

        /* glued doc */
        AssertMatched(*rp, R"(doc=/\/Search\/.+/; doc=/.*blah.*/)");
        AssertNotMatched(*rp, R"(doc!=/.*blah.*/; doc=/\/Search\/.+/)");
        AssertNotMatched(*rp, R"(doc=/\/Search\/.+/; doc!=/.*blah.*/)");

        /* glued ident_type */
        AssertMatched(*rp, R"(ident_type=/1-774646834/; ident_type=/1-.*/)");
        AssertMatched(*rp, R"(ident_type=/1-.*/; ident_type=/1-774646834/)");
        AssertNotMatched(*rp, R"(ident_type!=/1-774646834/; ident_type=/1-.*/)");
        AssertNotMatched(*rp, R"(ident_type=/1-774646834/; ident_type!=/1-.*/)");
        AssertNotMatched(*rp, R"(ident_type!=/1-774646834/; ident_type!=/1-.*/)");

        /* glued arrival_time */
        AssertMatched(*rp, R"(arrival_time=/.*135.*/; arrival_time=/.*889168.*/; arrival_time!=/.*6666.*/)");
        AssertNotMatched(*rp, R"(arrival_time!=/.*889168.*/; arrival_time!=/.*6666.*/)");
    }

    Y_UNIT_TEST(TestRuleDegradationWeb) {
        const TString spravkaKey = "4a1faf3281028650e82996f37aada9d97ef7c7c4f3c71914a7cc07a1cbb02d00";
        TStringInput spravkaSI(spravkaKey);
        TSpravkaKey::SetInstance(TSpravkaKey(spravkaSI));

        const TString testKey = "102c46d700bed5c69ed20b7473886468";
        TStringInput keys(testKey);
        TKeyRing::SetInstance(TKeyRing(keys));

        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST, {}, {}, GEOBASE_BIN_PATH);
        AssertNotMatched(*rp, R"(degradation=yes)");
        AssertMatched(*rp, R"(degradation=no)");


        THolder<TRequest> rpWithSpravka = CreateDummyParsedRequest(REQUEST_DEGR, {}, {}, GEOBASE_BIN_PATH);
        AssertMatched(*rpWithSpravka, R"(degradation=yes)");
        AssertNotMatched(*rpWithSpravka, R"(degradation=no)");
    }

    Y_UNIT_TEST(TestPanicCbb) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST, {}, {}, GEOBASE_BIN_PATH);
        AssertNotMatched(*rp, R"(panic_mode=yes)");
        AssertMatched(*rp, R"(panic_mode=no)");

        rp->CbbPanicMode = true;
        AssertMatched(*rp, R"(panic_mode=yes)");
        AssertNotMatched(*rp, R"(panic_mode=no)");
    }

    Y_UNIT_TEST(TestInRobotSet) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST, {}, {}, GEOBASE_BIN_PATH);
        AssertNotMatched(*rp, R"(in_robot_set=yes)");
        AssertMatched(*rp, R"(panic_mode=no)");

        rp->InRobotSet = true;
        AssertMatched(*rp, R"(in_robot_set=yes)");
        AssertNotMatched(*rp, R"(in_robot_set=no)");
    }

    Y_UNIT_TEST(TestRequestMatchingCbb) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST);
        AssertMatched(*rp, R"(request=/.*X-Forwarded-For-Y:.*X-Source-Port-Y:.*X-Req-Id:.*/)");
        AssertNotMatched(*rp, R"(request=/.*X-Forwarded-For-Y:.*X-Req-Id:.*X-Source-Port-Y:.*/)");
    }

    Y_UNIT_TEST(TestHodorCbb) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST);
        AssertMatched(*rp, R"(hodor=/be-cu.*bE/)");
        AssertNotMatched(*rp, R"(hodor=/be.*cu-bE/)");
        AssertMatched(*rp, R"(hodor_hash=/3001031346896947238/)");
    }

    Y_UNIT_TEST(TestJwsInfoCbb) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST);
        AssertNotMatched(*rp, R"(jws_info=/VALID/)");
        AssertMatched(*rp, R"(jws_info=/INVALID/)");
        AssertNotMatched(*rp, R"(jws_info=/SUSP/)");
        AssertNotMatched(*rp, R"(jws_info=/DEFAULT/)");
    }

    Y_UNIT_TEST(TestYandexTrustInfoCbb) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST);
        AssertNotMatched(*rp, R"(yandex_trust_info=/VALID/)");
        AssertMatched(*rp, R"(yandex_trust_info=/INVALID/)");
        AssertNotMatched(*rp, R"(yandex_trust_info=/AAAAAA/)");

        rp->YandexTrustState = EYandexTrustState::Valid;
        AssertMatched(*rp, R"(yandex_trust_info=/VALID/)");
        AssertNotMatched(*rp, R"(yandex_trust_info=/INVALID/)");
        AssertNotMatched(*rp, R"(yandex_trust_info=/AAAAAA/)");
    }

    Y_UNIT_TEST(TestRandomMatchingCbb) {
        THolder<TRequest> rp = CreateDummyParsedRequest(REQUEST);
        AssertNotMatched(*rp, R"(random=0)");
        AssertMatched(*rp, R"(random=100)");
        AssertNotMatched(*rp, R"(jws_info=/INVALID/; random=0)");
        AssertMatched(*rp, R"(jws_info=/INVALID/; random=100)");
        AssertNotMatched(*rp, R"(jws_info=/VALID/; random=100)");
    }

    Y_UNIT_TEST(TestFactorConditions) {
        const auto req = CreateDummyParsedRequest(REQUEST);

        TRawCacherFactors factors;
        factors.Factors[0] = 3;

        const auto factorName = ToString(ECacherFactors{0});
        const TString rule = std::format(
            "factor['{}']>1; factor['{}']<3.5",
            factorName.data(), factorName.data()
        );

        AssertMatched(*req, rule, &factors);

        factors.Factors[0] = 0.5;
        AssertNotMatched(*req, rule, &factors);
    }
}
