#pragma once

#include "addr.h"

#include <array>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

class TCgiParameters;
class THttpCookies;

namespace NAntiRobot {
    class TSpravka {
    public:
        enum class ECookieParseResult {
            NotFound,
            Invalid,
            Valid
        };

        struct TDegradation {
            bool Web = false;
            bool Market = false;
            bool Uslugi = false;
            bool Autoru = false;
        };

        ui64 Uid = 0;
        TAddr Addr;
        TInstant Time;
        TString Domain;

        TDegradation Degradation;

        TSpravka() = default;

        static TSpravka Generate(const TAddr& addr, TStringBuf domain, TDegradation degradation);
        static TSpravka Generate(const TAddr& addr, TInstant gen, TStringBuf domain, TDegradation degradation);
        static TInstant GetTime(ui64 uid);

        inline static constexpr std::array<TStringBuf, 2> NAMES{"spravka", "sprvk"};

        TString ToString() const;
        TString AsCookie() const;
        ui64 Hash() const;

        ECookieParseResult ParseCookies(const THttpCookies& cookies, TStringBuf domain);
        ECookieParseResult ParseCGI(const TCgiParameters& cgi, TStringBuf domain);
        bool Parse(TStringBuf spravkaBuf, TStringBuf domain);
    private:
        TSpravka(ui64 uid, const TAddr& addr, TInstant gen, TStringBuf domain, TDegradation degradation)
            : Uid(uid)
            , Addr(addr)
            , Time(gen)
            , Domain(domain)
            , Degradation(degradation)
        {
        }

        TString EncryptData() const;
        void DecryptData(TStringBuf hexData);
    };
} // namespace NAntiRobot
