#include <library/cpp/testing/unittest/registar.h>

#include "match_rule_lexer.h"

using namespace NAntiRobot::NMatchRequest;

#define TOKEN_FOLLOWED_END(str, tok)                                                    \
    {                                                                                   \
        TLexer lex(str);                                                       \
        UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, tok);                        \
        UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::End);       \
    }

#define TOKEN_UNEQUAL(str, tok)                                                         \
    {                                                                                   \
        TLexer lex(str);                                                       \
        UNIT_ASSERT_VALUES_UNEQUAL(lex.GetNextToken().TokenType, tok);                      \
    }

Y_UNIT_TEST_SUITE(TTestALLexer) {
    Y_UNIT_TEST(EmptyInput) {
        UNIT_ASSERT_VALUES_EQUAL(TLexer("").GetNextToken().TokenType, TLexer::End);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(" ").GetNextToken().TokenType, TLexer::End);
        UNIT_ASSERT_VALUES_EQUAL(TLexer("  ").GetNextToken().TokenType, TLexer::End);
        UNIT_ASSERT_VALUES_EQUAL(TLexer("\t  \n").GetNextToken().TokenType, TLexer::End);
    }

    Y_UNIT_TEST(TokenFollowedEnd) {
        TOKEN_FOLLOWED_END(R"(123)", TLexer::Unsigned);
        TOKEN_FOLLOWED_END(R"(yes)", TLexer::True);
        TOKEN_FOLLOWED_END(R"(no)", TLexer::False);
        TOKEN_FOLLOWED_END(R"(!)", TLexer::Char);
        TOKEN_FOLLOWED_END(R"(&)", TLexer::Char);
        TOKEN_FOLLOWED_END(R"( t)", TLexer::Ident);
        TOKEN_FOLLOWED_END(R"( host1)", TLexer::Ident);
        TOKEN_FOLLOWED_END(R"(; )", TLexer::Delim);
    }

    Y_UNIT_TEST(Ip4) {
        TOKEN_FOLLOWED_END(R"(1.2.3.4)", TLexer::Cidr4);
        TOKEN_FOLLOWED_END(R"(1.2.3.4/8)", TLexer::Cidr4);
    }

    Y_UNIT_TEST(Ip6) {
        TOKEN_FOLLOWED_END(R"(::1234:1abc:1)", TLexer::Cidr6);
        TOKEN_FOLLOWED_END(R"(10::1234:1abc:1/64)", TLexer::Cidr6);
        TOKEN_FOLLOWED_END(R"(::1.2.3.4/16)", TLexer::Cidr6);
        TOKEN_FOLLOWED_END(R"(ffff::1.2.3.4/16)", TLexer::Cidr6);
        TOKEN_FOLLOWED_END(R"(1234::1234)", TLexer::Cidr6);

        TOKEN_UNEQUAL(R"(:::ffff)", TLexer::Cidr6);
        TOKEN_UNEQUAL(R"(::gfff)", TLexer::Cidr6);
        TOKEN_UNEQUAL(R"(12345::1234)", TLexer::Cidr6);
    }

    Y_UNIT_TEST(NotTokenInsideIdent) {
        TLexer lex(R"(nostradamus)");
        UNIT_ASSERT_VALUES_UNEQUAL(lex.GetNextToken().TokenType, TLexer::False);
    }

    Y_UNIT_TEST(Numbers) {
        TOKEN_FOLLOWED_END(R"(1)", TLexer::Unsigned);
        TOKEN_FOLLOWED_END(R"(+123)", TLexer::Integer);
        TOKEN_FOLLOWED_END(R"(-321)", TLexer::Integer);
        TOKEN_FOLLOWED_END(R"(1.0)", TLexer::Float);
        TOKEN_FOLLOWED_END(R"(1.123)", TLexer::Float);
        TOKEN_FOLLOWED_END(R"(+1.123)", TLexer::Float);
        TOKEN_FOLLOWED_END(R"(-1.123)", TLexer::Float);
    }

    Y_UNIT_TEST(Keyword) {
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(rule_id)").GetNextToken().TokenType, TLexer::RuleId);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(doc)").GetNextToken().TokenType, TLexer::Doc);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(enabled)").GetNextToken().TokenType, TLexer::Enabled);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(ip)").GetNextToken().TokenType, TLexer::Ip);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(no)").GetNextToken().TokenType, TLexer::False);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(nonblock)").GetNextToken().TokenType, TLexer::Nonblock);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(header)").GetNextToken().TokenType, TLexer::Header);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(has_header)").GetNextToken().TokenType, TLexer::HasHeader);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(num_header)").GetNextToken().TokenType, TLexer::NumHeader);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(csheader)").GetNextToken().TokenType, TLexer::CSHeader);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(has_csheader)").GetNextToken().TokenType, TLexer::HasCSHeader);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(num_csheader)").GetNextToken().TokenType, TLexer::NumCSHeader);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(factor)").GetNextToken().TokenType, TLexer::Factor);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(yes)").GetNextToken().TokenType, TLexer::True);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(ip_from)").GetNextToken().TokenType, TLexer::IpFrom);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(ident_type)").GetNextToken().TokenType, TLexer::IdentType);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(arrival_time)").GetNextToken().TokenType, TLexer::ArrivalTime);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(service_type)").GetNextToken().TokenType, TLexer::ServiceType);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_tor)").GetNextToken().TokenType, TLexer::IsTor);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_proxy)").GetNextToken().TokenType, TLexer::IsProxy);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_vpn)").GetNextToken().TokenType, TLexer::IsVpn);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_hosting)").GetNextToken().TokenType, TLexer::IsHosting);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_mobile)").GetNextToken().TokenType, TLexer::IsMobile);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_whitelist)").GetNextToken().TokenType, TLexer::IsWhitelist);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(country_id)").GetNextToken().TokenType, TLexer::CountryId);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(degradation)").GetNextToken().TokenType, TLexer::Degradation);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(panic_mode)").GetNextToken().TokenType, TLexer::PanicMode);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(request)").GetNextToken().TokenType, TLexer::Request);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(in_robot_set)").GetNextToken().TokenType, TLexer::InRobotSet);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(hodor)").GetNextToken().TokenType, TLexer::Hodor);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(hodor_hash)").GetNextToken().TokenType, TLexer::HodorHash);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_mikrotik)").GetNextToken().TokenType, TLexer::IsMikrotik);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_squid)").GetNextToken().TokenType, TLexer::IsSquid);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(is_ddoser)").GetNextToken().TokenType, TLexer::IsDdoser);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(jws_info)").GetNextToken().TokenType, TLexer::JwsInfo);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(yandex_trust_info)").GetNextToken().TokenType, TLexer::YandexTrustInfo);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(random)").GetNextToken().TokenType, TLexer::Random);
        UNIT_ASSERT_VALUES_EQUAL(TLexer(R"(may_ban)").GetNextToken().TokenType, TLexer::MayBan);
        UNIT_ASSERT_VALUES_EQUAL(TLexer("exp_bin").GetNextToken().TokenType, TLexer::ExpBin);
    }

    Y_UNIT_TEST(Operators) {
        TOKEN_FOLLOWED_END(R"(=)", TLexer::Assign);
        TOKEN_FOLLOWED_END(R"(==)", TLexer::Equal);
        TOKEN_FOLLOWED_END(R"(!=)", TLexer::NotEqual);
        TOKEN_FOLLOWED_END(R"(>)", TLexer::More);
        TOKEN_FOLLOWED_END(R"(>=)", TLexer::MoreEqual);
        TOKEN_FOLLOWED_END(R"(<)", TLexer::Less);
        TOKEN_FOLLOWED_END(R"(<=)", TLexer::LessEqual);
    }

    Y_UNIT_TEST(ValueOfChar)
    {
        {
            TLexer lex(R"($)");
            TLexer::TTokValue tokval = lex.GetNextToken();
            UNIT_ASSERT_VALUES_EQUAL(tokval.TokenType, TLexer::Char);
            UNIT_ASSERT_STRINGS_EQUAL(tokval.Value, R"($)");
        }
        {
            TLexer lex(R"([)");
            TLexer::TTokValue tokval = lex.GetNextToken();
            UNIT_ASSERT_VALUES_EQUAL(tokval.TokenType, TLexer::Char);
            UNIT_ASSERT_STRINGS_EQUAL(tokval.Value, R"([)");
        }
    }

    Y_UNIT_TEST(String) {
        const char* samples[] = {
            R"('doc')",
            R"("doc")",
            R"("ho \"st")",
            R"("ho'st")",
            R"('ho\'st')",
            R"('ho"st')",
        };

        for (const auto& sample: samples) {
            TLexer lex(sample);
            TLexer::TTokValue tokval = lex.GetNextToken();
            UNIT_ASSERT_VALUES_EQUAL_C(tokval.TokenType, TLexer::String, "");
            UNIT_ASSERT_STRINGS_EQUAL_C(tokval.Value, sample, "");
        }
    }

    Y_UNIT_TEST(Regex) {
        const char* samples[] = {
            R"(/host/)",
            R"(/host/i)",
            R"(/.*i\/\d+/)",
        };

        for (const auto& sample: samples) {
            TLexer lex(sample);
            TLexer::TTokValue tokval = lex.GetNextToken();
            UNIT_ASSERT_VALUES_EQUAL_C(tokval.TokenType, TLexer::Regex, "");
            UNIT_ASSERT_STRINGS_EQUAL_C(tokval.Value, sample, "");
        }

        {
            TLexer lex(R"(/.*i/ ; /\d+/i)");
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Regex);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Regex);
        }
    }

    Y_UNIT_TEST(LastToken) {
        TLexer lex(R"(doc; ip)");

        UNIT_ASSERT_VALUES_EQUAL(lex.GetCurrentToken().TokenType , TLexer::NotAToken);
        UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Doc);
        UNIT_ASSERT_VALUES_EQUAL(lex.GetCurrentToken().TokenType , TLexer::Doc);

        UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);
        UNIT_ASSERT_VALUES_EQUAL(lex.GetCurrentToken().TokenType , TLexer::Delim);

        UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Ip);
        UNIT_ASSERT_VALUES_EQUAL(lex.GetCurrentToken().TokenType, TLexer::Ip);

        UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::End);
        UNIT_ASSERT_VALUES_EQUAL(lex.GetCurrentToken().TokenType, TLexer::End);
    }

    Y_UNIT_TEST(Sequence) {
        {
            TLexer lex(R"( yes;doc;  no;)");

            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::True);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Doc);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::False);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::End);
        }
        {
            TLexer lex(R"(rem='Test';enabled = yes; header['host']=/.*.yandex.ru/i)");

            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Rem);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Assign);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::String);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);

            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Enabled);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Assign);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::True);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);

            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Header);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Char);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::String);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Char);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Assign);
            {
                TLexer::TTokValue tokval = lex.GetNextToken();
                UNIT_ASSERT_VALUES_EQUAL(tokval.TokenType, TLexer::Regex);
                UNIT_ASSERT_VALUES_EQUAL(tokval.Value, R"(/.*.yandex.ru/i)");
            }

            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::End);
        }
        {
            TLexer lex(R"(request=/.*Accept-Language:.*User-Agent:.*Cookie:.*Accept-Language:ru.*/;)");

            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Request);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Assign);
            {
                TLexer::TTokValue tokval = lex.GetNextToken();
                UNIT_ASSERT_VALUES_EQUAL(tokval.Value, R"(/.*Accept-Language:.*User-Agent:.*Cookie:.*Accept-Language:ru.*/)");
                UNIT_ASSERT_VALUES_EQUAL(tokval.TokenType, TLexer::Regex);
            }
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::Delim);
            UNIT_ASSERT_VALUES_EQUAL(lex.GetNextToken().TokenType, TLexer::End);
        }
    }
}
#undef TOKEN_FOLLOWED_END
