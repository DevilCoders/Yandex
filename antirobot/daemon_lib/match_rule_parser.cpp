#include "match_rule_parser.h"

#include "match_rule_lexer.h"

#include <antirobot/lib/enum.h>

#include <util/generic/cast.h>
#include <util/string/subst.h>

namespace NAntiRobot {
    namespace {
        using namespace NMatchRequest;

        void ParseOP(TLexer& lexer, TRule& rule);
        void ParseNameValue(TLexer& lexer, std::function<void ()> valueReader);
        void ParseHeaderValue(
            TLexer& lexer,
            THashMap<TString, TVector<TRegexCondition>>& headers,
            bool caseSensitive = true
        );
        void ParseHasKeyInDictValue(TLexer& lexer, TVector<THasHeaderCondition>& numHeaders);
        void ParseNumHeaderValue(TLexer& lexer, TVector<TNumHeaderCondition>& numHeaders);
        void ParseFactor(TLexer& lexer, TVector<TFactorCondition>& factorConditions);

#define THROW_PARSE_ERROR(lexer, expected) ythrow TParseRuleError() << expected << ", <" << lexer.GetCurrentToken().TokenType << "> found";

        EComparatorOp TokenToComparatorOp(const TLexer& lexer, TLexer::EToken tokenType) {
            switch (tokenType) {
                case TLexer::Equal:
                    return EComparatorOp::Equal;
                case TLexer::NotEqual:
                    return EComparatorOp::NotEqual;
                case TLexer::More:
                    return EComparatorOp::Greater;
                case TLexer::MoreEqual:
                    return EComparatorOp::GreaterEqual;
                case TLexer::Less:
                    return EComparatorOp::Less;
                case TLexer::LessEqual:
                    return EComparatorOp::LessEqual;
                default:
                    THROW_PARSE_ERROR(lexer, "compare operator expected");
            }
        }

        TString StripQuote(const TString& str) {
            TString res = str;
            if (str[0] == '"' || str[0] == '\'') {
                res.erase(0, 1);

                if (str.back() == '"' || str.back() == '\'') {
                    res.pop_back();
                }
            }

            return res;
        }

        TString ReadString(TLexer& lexer) {
            TLexer::TTokValue token = lexer.GetCurrentToken();

            if (token.TokenType != TLexer::String) {
                THROW_PARSE_ERROR(lexer, "string expected");
            }

            return StripQuote(token.Value);
        }

        bool ReadBool(TLexer& lexer) {
            TLexer::TTokValue token = lexer.GetCurrentToken();
            switch (token.TokenType) {
                case TLexer::False:
                    return false;

                case TLexer::True:
                    return true;

                default:
                    THROW_PARSE_ERROR(lexer, "'yes' or 'no' expected");
            }
        }

        unsigned ReadUnsigned(TLexer& lexer) {
            TLexer::TTokValue token = lexer.GetCurrentToken();

            switch (token.TokenType) {
                case TLexer::Unsigned:
                    return FromString<unsigned>(token.Value);

                default:
                    THROW_PARSE_ERROR(lexer, "integer value expected");
            }
        }

        float ReadFloat(TLexer& lexer) {
            TLexer::TTokValue token = lexer.GetCurrentToken();

            switch (token.TokenType) {
                case TLexer::Unsigned:
                case TLexer::Integer:
                    return FromString<long>(token.Value);

                case TLexer::Float:
                    return FromString<float>(token.Value);

                default:
                    THROW_PARSE_ERROR(lexer, "float or integer value expected");
            }
        }

        TRegexCondition ReadRegexValue(TLexer& lexer, bool matchEquality) {
            TRegexCondition res;

            TLexer::TTokValue token = lexer.GetCurrentToken();

            switch (token.TokenType) {
            case TLexer::Regex: {
                if (auto maybeValue = TRegexMatcherEntry::Parse(token.Value)) {
                    res.Value = *maybeValue;
                } else {
                    THROW_PARSE_ERROR(lexer, "invalid regex");
                }

                break;
            }

            case TLexer::Group:
                res.Value = FromString<TCbbGroupId>(TStringBuf(token.Value).Skip(1));
                break;

            default:
                THROW_PARSE_ERROR(lexer, "regex or group id expected");
                break;
            }

            res.ShouldMatch = matchEquality;

            return res;
        }

        TIpInterval ReadCidr(TLexer& lexer) {
            TRegexCondition res;

            TLexer::TTokValue token = lexer.GetCurrentToken();

            switch (token.TokenType) {
                case TLexer::Cidr4:
                case TLexer::Cidr6:
                    return TIpInterval::Parse(token.Value);

                default:
                    THROW_PARSE_ERROR(lexer, "ip in form of CIDR expected");
            }
        }

        void ParseNameValue(TLexer& lexer, std::function<void ()> valueReader) {
            TLexer::TTokValue token = lexer.GetNextToken();
            if (token.TokenType != TLexer::Assign) {
                THROW_PARSE_ERROR(lexer, "'=' expected");
            }

            lexer.GetNextToken();
            valueReader();
            lexer.GetNextToken();
        }

        void ParseNameValue(TLexer& lexer, std::function<void (bool)> valueReader) {
            TLexer::TTokValue token = lexer.GetNextToken();
            if (token.TokenType != TLexer::Assign && token.TokenType != TLexer::NotEqual) {
                THROW_PARSE_ERROR(lexer, "'=' or '!=' expected");
            }

            lexer.GetNextToken();
            valueReader(token.TokenType == TLexer::Assign);
            lexer.GetNextToken();
        }

        void ParseCookieAge(TLexer& lexer, TRule& rule) {
            lexer.GetNextToken();
            const TLexer::TTokValue cmpSign = lexer.GetCurrentToken();
            const auto op = TokenToComparatorOp(lexer, cmpSign.TokenType);

            lexer.GetNextToken();
            const auto constant = ReadFloat(lexer);

            rule.CookieAge = {op, constant};

            lexer.GetNextToken();
        }

        void ParseCurrentTimestamp(TLexer& lexer, TRule& rule) {
            const unsigned SECONDS_IN_DAY = 86400;

            lexer.GetNextToken();
            const TLexer::TTokValue cmpSign = lexer.GetCurrentToken();
            const auto op = TokenToComparatorOp(lexer, cmpSign.TokenType);

            lexer.GetNextToken();
            const auto constant = ReadUnsigned(lexer) % SECONDS_IN_DAY;

            rule.CurrentTimestamp.push_back({op, constant});

            lexer.GetNextToken();
        }

        TRule ParseRuleText(const TString& ruleText) {
            TLexer lex(ruleText);

            TRule rule;

            try {
                lex.GetNextToken();
                while(lex.GetCurrentToken().TokenType != TLexer::End) {
                    //  RULE ::= OP (DELIM OP)*
                    ParseOP(lex, rule);

                    if (lex.GetCurrentToken().TokenType == TLexer::End) {
                        break;
                    }

                    if (lex.GetCurrentToken().TokenType != TLexer::Delim) {
                        THROW_PARSE_ERROR(lex, "ThrowParseError: delimiter expected");
                    }
                    lex.GetNextToken();
                }
                return rule;
            } catch (TParseRuleError& ex) {
                ex << ", text being parsed: " << ruleText;
                throw ex;
            }
        }

        void ParseOP(TLexer& lexer, TRule& rule) {
            TLexer::TTokValue token = lexer.GetCurrentToken();

            switch (token.TokenType) {
                case TLexer::RuleId:
                    ParseNameValue(lexer, [&](){rule.RuleId = {ReadUnsigned(lexer)};});
                    break;

                case TLexer::Rem:
                    // Ignore.
                    ParseNameValue(lexer, [&](){/* rule.Rem = */ ReadString(lexer);});
                    break;

                case TLexer::Nonblock:
                    ParseNameValue(lexer, [&](){rule.Nonblock = ReadBool(lexer);});
                    break;

                case TLexer::Enabled:
                    ParseNameValue(lexer, [&](){rule.Enabled = ReadBool(lexer);});
                    break;

                case TLexer::Doc:
                    ParseNameValue(lexer, [&](bool equal){rule.Doc.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::Cgi:
                    ParseNameValue(lexer, [&](bool equal){rule.CgiString.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::IdentType:
                    ParseNameValue(lexer, [&](bool equal){rule.IdentType.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::ArrivalTime:
                    ParseNameValue(lexer, [&](bool equal){rule.ArrivalTime.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::ServiceType:
                    ParseNameValue(lexer, [&](bool equal){rule.ServiceType.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::IsTor:
                    ParseNameValue(lexer, [&](){rule.IsTor = ReadBool(lexer);});
                    break;

                case TLexer::IsProxy:
                    ParseNameValue(lexer, [&](){rule.IsProxy = ReadBool(lexer);});
                    break;

                case TLexer::IsVpn:
                    ParseNameValue(lexer, [&](){rule.IsVpn = ReadBool(lexer);});
                    break;

                case TLexer::IsHosting:
                    ParseNameValue(lexer, [&](){rule.IsHosting = ReadBool(lexer);});
                    break;

                case TLexer::IsMikrotik:
                    ParseNameValue(lexer, [&](){rule.IsMikrotik = ReadBool(lexer);});
                    break;

                case TLexer::IsSquid:
                    ParseNameValue(lexer, [&](){rule.IsSquid = ReadBool(lexer);});
                    break;

                case TLexer::IsDdoser:
                    ParseNameValue(lexer, [&](){rule.IsDdoser = ReadBool(lexer);});
                    break;

                case TLexer::IsMobile:
                    ParseNameValue(lexer, [&](){rule.IsMobile = ReadBool(lexer);});
                    break;

                case TLexer::IsWhitelist:
                    ParseNameValue(lexer, [&](){rule.IsWhitelist = ReadBool(lexer);});
                    break;

                case TLexer::CountryId:
                    ParseNameValue(lexer, [&](bool equal){rule.CountryId.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::Header:
                    ParseHeaderValue(lexer, rule.Headers, false);
                    break;

                case TLexer::HasHeader:
                    ParseHasKeyInDictValue(lexer, rule.HasHeaders);
                    break;

                case TLexer::NumHeader:
                    ParseNumHeaderValue(lexer, rule.NumHeaders);
                    break;

                case TLexer::CSHeader:
                    ParseHeaderValue(lexer, rule.CsHeaders, true);
                    break;

                case TLexer::HasCSHeader:
                    ParseHasKeyInDictValue(lexer, rule.HasCsHeaders);
                    break;

                case TLexer::NumCSHeader:
                    ParseNumHeaderValue(lexer, rule.NumCsHeaders);
                    break;

                case TLexer::Factor:
                    ParseFactor(lexer, rule.Factors);
                    break;

                case TLexer::Ip:
                    ParseNameValue(lexer, [&](){rule.IpInterval = ReadCidr(lexer);});
                    break;

                case TLexer::IpFrom:
                    ParseNameValue(lexer, [&](){rule.CbbGroup = SafeIntegerCast<TCbbGroupId>(ReadUnsigned(lexer));});
                    break;

                case TLexer::CookieAge:
                    ParseCookieAge(lexer, rule);
                    break;

                case TLexer::CurrentTimestamp:
                    ParseCurrentTimestamp(lexer, rule);
                    break;

                case TLexer::Degradation:
                    ParseNameValue(lexer, [&](){rule.Degradation = ReadBool(lexer);});
                    break;

                case TLexer::PanicMode:
                    ParseNameValue(lexer, [&](){rule.PanicMode = ReadBool(lexer);});
                    break;

                case TLexer::Request:
                    ParseNameValue(lexer, [&](bool equal){rule.Request.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::InRobotSet:
                    ParseNameValue(lexer, [&](){rule.InRobotSet = ReadBool(lexer);});
                    break;

                case TLexer::Hodor:
                    ParseNameValue(lexer, [&](bool equal){rule.Hodor.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::HodorHash:
                    ParseNameValue(lexer, [&](bool equal){rule.HodorHash.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::JwsInfo:
                    ParseNameValue(lexer, [&](bool equal){rule.JwsInfo.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::YandexTrustInfo:
                    ParseNameValue(lexer, [&](bool equal){rule.YandexTrustInfo.push_back(ReadRegexValue(lexer, equal));});
                    break;

                case TLexer::Random:
                    ParseNameValue(lexer, [&](){rule.RandomThreshold = ReadFloat(lexer) / 100.0;});
                    break;

                case TLexer::MayBan:
                    ParseNameValue(lexer, [&](){rule.MayBan = ReadBool(lexer);});
                    break;

                case TLexer::ExpBin:
                    ParseNameValue(lexer, [&](){
                        const auto expBin = ReadUnsigned(lexer);
                        if (expBin >= EnumValue(EExpBin::Count)) {
                            THROW_PARSE_ERROR(lexer, "expected an integer in range [0; 5]");
                        }
                        rule.ExpBin = static_cast<EExpBin>(expBin);
                    });
                    break;

                case TLexer::ValidAutoRuTamper:
                    ParseNameValue(lexer, [&](){rule.ValidAutoRuTamper = ReadBool(lexer);});
                    break;

                default:
                    THROW_PARSE_ERROR(lexer, "expected 'NAME=VALUE' or 'NAME!=VALUE' clause");
            }
        }

        void ParseHeaderValue(
            TLexer& lexer,
            THashMap<TString, TVector<TRegexCondition>>& headers,
            bool caseSensitive
        ) {
            TLexer::TTokValue tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != '[') {
                THROW_PARSE_ERROR(lexer, "expected '['");
            }

            lexer.GetNextToken();

            auto key = ReadString(lexer);

            if (!caseSensitive) {
                key.to_lower();
            }

            tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != ']') {
                THROW_PARSE_ERROR(lexer, "expected ']'");
            }

            TRegexCondition value;
            ParseNameValue(lexer, [&](bool equal){value = ReadRegexValue(lexer, equal);});

            headers[key].push_back(std::move(value));
        }

        void ParseHasKeyInDictValue(TLexer& lexer, TVector<THasHeaderCondition>& numHeaders) {
            TLexer::TTokValue tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != '[') {
                THROW_PARSE_ERROR(lexer, "expected '['");
            }

            lexer.GetNextToken();

            TString headerName = ReadString(lexer);

            tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != ']') {
                THROW_PARSE_ERROR(lexer, "expected ']'");
            }

            bool boolVal = false;
            ParseNameValue(lexer, [&] { boolVal = ReadBool(lexer);});

            numHeaders.push_back(THasHeaderCondition(std::move(headerName), boolVal));
        }

        void ParseNumHeaderValue(TLexer& lexer, TVector<TNumHeaderCondition>& numHeaders) {
            TLexer::TTokValue tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != '[') {
                THROW_PARSE_ERROR(lexer, "expected '['");
            }

            lexer.GetNextToken();

            TString headerName = ReadString(lexer);

            tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != ']') {
                THROW_PARSE_ERROR(lexer, "expected ']'");
            }

            ui32 count = 0;
            ParseNameValue(lexer, [&] { count = ReadUnsigned(lexer);});

            numHeaders.push_back(TNumHeaderCondition(headerName, count));
        }

        void ParseFactor(TLexer& lexer, TVector<TFactorCondition>& factorConditions) {
            auto tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != '[') {
                THROW_PARSE_ERROR(lexer, "expected '['");
            }

            lexer.GetNextToken();

            auto factorName = ReadString(lexer);

            tok = lexer.GetNextToken();
            if (tok.TokenType != TLexer::Char || tok.Value[0] != ']') {
                THROW_PARSE_ERROR(lexer, "expected ']");
            }

            lexer.GetNextToken();
            const TLexer::TTokValue cmpSign = lexer.GetCurrentToken();
            const auto op = TokenToComparatorOp(lexer, cmpSign.TokenType);

            lexer.GetNextToken();
            const auto constant = ReadFloat(lexer);

            factorConditions.push_back({factorName, {op, constant}});

            lexer.GetNextToken();
        }
    } // anonymous namespace

    namespace NMatchRequest {
        TRule ParseRule(const TString& ruleString) {
            return ParseRuleText(ruleString);
        }
    }
} // namespace NAntiRobot
