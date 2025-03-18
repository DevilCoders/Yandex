#pragma once

#include <util/generic/string.h>


namespace NAntiRobot {


namespace NMatchRequest {
    class TLexer {
    public:
        enum EToken {
            NotAToken = 0,
            End,
            Delim,
            Char,
            String,
            Regex,
            Unsigned,
            Integer,
            Float,
            Ident,
            Group,
            Cidr4,
            Cidr6,

            /* operators */
            Assign,
            Equal,
            NotEqual,
            More,
            MoreEqual,
            Less,
            LessEqual,

            /* keywords */
            RuleId,
            Doc,
            Enabled,
            False,
            Ip,
            IpFrom,
            Nonblock,
            Rem,
            True,
            Cgi,
            Header,
            HasHeader,
            NumHeader,
            CSHeader,
            HasCSHeader,
            NumCSHeader,
            Factor,
            IdentType,
            ArrivalTime,
            ServiceType,
            IsTor,
            IsProxy,
            IsVpn,
            IsHosting,
            IsMobile,
            IsWhitelist,
            CountryId,
            Degradation,
            PanicMode,
            Request,
            InRobotSet,
            Hodor,
            HodorHash,
            IsMikrotik,
            IsSquid,
            IsDdoser,
            JwsInfo,
            YandexTrustInfo,
            Random,
            MayBan,
            ExpBin,
            ValidAutoRuTamper,
            CookieAge,
            CurrentTimestamp,
        };

        struct TTokValue {
            EToken TokenType;
            TString Value;
        };

        TLexer(const TString& text);
        TTokValue GetNextToken();
        TTokValue GetCurrentToken() const {
            return LastToken;
        }
    private:
        TString Text;
        TTokValue LastToken;
    private:
        /* Ragel related */
        int cs;
        int act;
        char* ts;
        char* te;
        char* p;
        char* pe;
        char* eof;
    };
} // namespace NMatchRequest


} // namespace NAntiRobot
