#pragma once

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

class TInstant;

namespace NAntiRobot {
    enum class ECaptchaType {
        Normal         /* "0" */,  // TODO: почему-то после удаления всё ломается
        SmartKeys      /* "1" */,
        SmartCheckbox  /* "2" */,
        SmartAdvanced  /* "3" */,
    };

    class TProxiedCaptchaUrlBad : public yexception {
    };

    struct TCaptchaToken {
        ECaptchaType CaptchaType;
        TInstant Timestamp;
        TString AsString;

        TCaptchaToken();

        TCaptchaToken(ECaptchaType captchaType, TStringBuf uniqueKey);

        /// @throw yexception if fails to parse
        static TCaptchaToken Parse(TStringBuf token);
        TCaptchaToken WithType(ECaptchaType captchaType);
    private:
        TCaptchaToken(ECaptchaType captchaType, TInstant timestamp, TStringBuf asString);
    };

    inline bool operator == (const TCaptchaToken& token1, const TCaptchaToken& token2) {
        return token1.CaptchaType == token2.CaptchaType && token1.Timestamp == token2.Timestamp;
    }

    struct TCaptchaKey {
        TString ImageKey;
        TCaptchaToken Token;
        TStringBuf Host;

        class TParseError : public yexception {
        };

        /// @throw TParseError if fails to parse
        static TCaptchaKey Parse(const TStringBuf& keySigned, const TStringBuf& host);
        TString ToString() const;
    };

    TString MakeProxiedCaptchaUrl(const TString& realImgUrl, const TCaptchaToken& token, const TStringBuf& newLocation);
    void ParseProxiedCaptchaUrl(const TStringBuf& proxiedImgUrl, TString& realImgUrl, TStringBuf& token, bool checkSignature);
}
