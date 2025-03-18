#include "captcha_key.h"
#include "config_global.h"

#include <antirobot/lib/keyring.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/datetime/base.h>
#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/random/random.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

namespace NAntiRobot {

    /*
     * TCaptchaToken
     */
    namespace {

    TString MakeCaptchaToken(TStringBuf uniqueKey, const TInstant& timestamp, ECaptchaType captchaType) {
        const size_t MD5_SIGN_LEN = 32;

        TTempBufOutput token;
        ui64 seconds = timestamp.Seconds();

        token << ToString(captchaType) << '/' << seconds << '/';
        MD5::Data((const unsigned char*)uniqueKey.data(), uniqueKey.size(), token.Current());
        token.Proceed(MD5_SIGN_LEN);
        return TString(token.Data(), token.Filled());
    }

    }

    TCaptchaToken::TCaptchaToken() : CaptchaType(ECaptchaType::SmartCheckbox) {
    }

    TCaptchaToken::TCaptchaToken(ECaptchaType captchaType, TStringBuf uniqueKey)
        : CaptchaType(captchaType)
        , Timestamp(TInstant::Now())
        , AsString(MakeCaptchaToken(uniqueKey, Timestamp, captchaType))
    {
    }

    TCaptchaToken::TCaptchaToken(ECaptchaType captchaType, TInstant timestamp,
                                 TStringBuf asString)
        : CaptchaType(captchaType)
        , Timestamp(timestamp)
        , AsString(asString)
    {
    }

    TCaptchaToken TCaptchaToken::Parse(TStringBuf token) {
        TStringBuf rcBuf;
        TStringBuf tail;
        if (!token.TrySplit('/', rcBuf, tail) || rcBuf.size() != 1) {
            ythrow yexception() << "Invalid token: " << token;
        }

        TStringBuf tsBuf;
        if (!tail.TrySplit('/', tsBuf, tail) || tsBuf.empty()) {
            ythrow yexception() << "Invalid token: " << token;
        }

        ECaptchaType captchaType;
        if (!TryFromString<ECaptchaType>(rcBuf, captchaType)) {
            ythrow yexception() << "Invalid token: " << token;
        }

        TInstant timeStamp = TInstant::Seconds(FromString<ui64>(tsBuf));
        return TCaptchaToken(captchaType, timeStamp, token);
    }

    TCaptchaToken TCaptchaToken::WithType(ECaptchaType captchaType) {
        auto pos = AsString.find('/');
        TString newString = pos == TString::npos ? AsString : ToString(int(captchaType)) + AsString.substr(pos);
        return TCaptchaToken(captchaType, Timestamp, newString);
    }

    /*
     * TCaptchaKey
     */
    TCaptchaKey TCaptchaKey::Parse(const TStringBuf& keySigned, const TStringBuf& host)
    try {
        TStringBuf keyBuf(keySigned);

        size_t splitPos = keyBuf.rfind('_');
        if (splitPos == TStringBuf::npos) {
            ythrow TParseError() << "Symbol '_' wasn't found in " << keySigned;
        }

        TStringBuf signature = keyBuf.SubStr(splitPos + 1);
        if (signature.empty()) {
            ythrow TParseError() << "No signature in " << keySigned;
        }

        TStringBuf keyWithToken = keyBuf.SubStr(0, splitPos);

        TTempBuf bufForCheck(keyWithToken.size() + host.size() + 2);
        bufForCheck.Append(keyWithToken.data(), keyWithToken.size());
        bufForCheck.Append(host.data(), host.size());

        bool isSigned = TKeyRing::Instance()->IsSignedHex(TStringBuf(bufForCheck.Data(), bufForCheck.Filled()), signature);
        // Этот кусок нужен для обратной совместимости на время выкатики
        // -- start
        for (size_t successModificatorId = 0; successModificatorId < 4; ++successModificatorId) {
            const TString idStr = ::ToString(successModificatorId);
            bufForCheck.Append(idStr.data(), idStr.size());

            const TStringBuf bufStr(bufForCheck.Data(), bufForCheck.Filled());
            if (TKeyRing::Instance()->IsSignedHex(bufStr, signature)) {
                isSigned = true;
                break;
            }
            bufForCheck.SetPos(bufForCheck.Filled() - idStr.size());
        }
        // -- end
        if (!isSigned) {
            ythrow TParseError() << keySigned << " has invalid signature";
        }

        size_t splitPos2 = keyWithToken.rfind('_');
        if (splitPos2 == TStringBuf::npos) {
            ythrow TParseError() << "There's only one symbol '_' in " << keySigned;
        }

        TStringBuf key(keyWithToken.data(), splitPos2);
        TStringBuf token(keyWithToken.data() + splitPos2 + 1, keyWithToken.size() - splitPos2 - 1);

        return {TString{key}, TCaptchaToken::Parse(token), host};
    } catch (const TParseError&) {
        throw;
    } catch (...) {
        ythrow TParseError() << "Failed to parse " << keySigned << ": " << CurrentExceptionMessage();
    }

    TString TCaptchaKey::ToString() const {
        const TString& tokenStr = Token.AsString;

        TTempBuf keyWithSign(ImageKey.size() + tokenStr.size() + 2 + TSecretKey::SignHexLen);
        keyWithSign.Append(ImageKey.data(), ImageKey.size());
        keyWithSign.Append("_", 1);
        keyWithSign.Append(tokenStr.data(), tokenStr.size());

        TTempBuf bufForSign(keyWithSign.Filled() + Host.size() + 2);
        bufForSign.Append(keyWithSign.Data(), keyWithSign.Filled());
        bufForSign.Append(Host.data(), Host.size());
        size_t successModificatorId = 0; // TODO: удалить из подписи
        const TString idStr = ::ToString(successModificatorId);
        bufForSign.Append(idStr.data(), idStr.size());

        const TString sign = TKeyRing::Instance()->SignHex(TStringBuf(bufForSign.Data(), bufForSign.Filled()));
        keyWithSign.Append("_", 1);
        keyWithSign.Append(sign.data(), sign.size());

        return TString(keyWithSign.Data(), keyWithSign.Filled());
    }

    /*
     * Other stuff
     */
    TString MakeProxiedCaptchaUrl(const TString& realImgUrl, const TCaptchaToken& token,
                                 const TStringBuf& newLocation)
    {
        TTempBuf encBuf;
        TStringBuf encodedUrl = Base64EncodeUrl(TStringBuf(realImgUrl.data(), realImgUrl.size()), encBuf.Data());

        TTempBufOutput proxiedImgUrl;
        proxiedImgUrl << newLocation << '?' << encodedUrl << '_' << token.AsString;

        const auto sign = TKeyRing::Instance()->SignHex(TStringBuf(proxiedImgUrl.Data(), proxiedImgUrl.Filled()));
        proxiedImgUrl << '_' << sign;

        return TString(proxiedImgUrl.Data(), proxiedImgUrl.Filled());
    }


    void ParseProxiedCaptchaUrl(const TStringBuf& proxiedImgUrl, TString& realImgUrl, TStringBuf& token, bool checkSignature) {
        static const char* BAD_FORMAT = "Bad image url format";

        size_t splitPos = proxiedImgUrl.rfind('_');
        if (splitPos == TStringBuf::npos)
            ythrow TProxiedCaptchaUrlBad() << BAD_FORMAT;

        TStringBuf signature = proxiedImgUrl.SubStr(splitPos + 1);
        if (signature.empty())
            ythrow TProxiedCaptchaUrlBad() << BAD_FORMAT;

        TStringBuf urlWithToken = proxiedImgUrl.SubStr(0, splitPos);

        if (checkSignature && !TKeyRing::Instance()->IsSignedHex(urlWithToken, signature))
            ythrow TProxiedCaptchaUrlBad() << "Bad image url signature";

        size_t splitPos2 = urlWithToken.rfind('_');
        if (splitPos2 == TStringBuf::npos)
            ythrow TProxiedCaptchaUrlBad() << "Bad url-with-token format";

        TStringBuf encodedUrl = TStringBuf(urlWithToken.data(), splitPos2).After('?');
        realImgUrl = Base64Decode(encodedUrl);
        token = TStringBuf(urlWithToken.data() + splitPos2 + 1, urlWithToken.size() -  splitPos2 - 1);
    }
}
