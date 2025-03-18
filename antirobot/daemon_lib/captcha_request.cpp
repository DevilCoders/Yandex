#include "captcha_request.h"

#include "environment.h"
#include "eventlog_err.h"
#include "request_context.h"
#include "request_params.h"

#include <antirobot/lib/evp.h>
#include <antirobot/lib/keyring.h>
#include <antirobot/lib/range.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/join.h>
#include <util/string/split.h>


namespace NAntiRobot {


namespace {
    void ExtractFingerprint(
        const TFeaturedAddr& userAddr,
        const TCgiParameters& post,
        TCaptchaRequest::TBrowserJsPrint* fingerprint
    ) {
        TStringBuf version;
        TStringBuf timeStr;
        TStringBuf rnd;
        TStringBuf signature;

        ParseSplitTokensInto(
            StringSplitter(post.Get("k")).Split('_'),
            &version, &timeStr, &rnd, &signature
        );

        Y_ENSURE(version == "1", "Unknown fingerprint version");

        const auto time = TInstant::Seconds(FromString<ui64>(timeStr));
        const auto now = TInstant::Now();
        Y_ENSURE(
            time <= now && now - time < ANTIROBOT_DAEMON_CONFIG.CaptchaRequestKeyLifetime,
            "Invalid or old fingerprint timestamp"
        );

        const auto& encodedKey = post.Get("d");
        const auto signatureData = Join('_', encodedKey, "1", timeStr, userAddr.ToString(), rnd);

        Y_ENSURE(
            TKeyRing::Instance()->IsSignedHex(signatureData, signature),
            "Invalid fingerprint signature"
        );

        const auto key = Base64StrictDecode(encodedKey);
        auto fingerprintData = Base64StrictDecode(post.Get("rdata"));

        for (size_t i = 0; i < fingerprintData.size(); ++i) {
            fingerprintData[i] = fingerprintData[i] ^ key[i % key.size()];
        }

        fingerprint->ParseFromJsonString(fingerprintData);
    }

    TCgiParameters GetPostParameters(const TRequest& request) {
        NJson::TJsonValue json;
        if (request.ContentData.StartsWith("{") && NJson::ReadJsonTree(request.ContentData, &json) && json.IsMap()) {
            TCgiParameters result;
            for (const auto& [k, v] : json.GetMapSafe()) {
                if (v.IsString()) {
                    result.insert({k, v.GetStringSafe()});
                }
            }
            return result;
        }
        return TCgiParameters(request.ContentData);
    }

    ELanguage GetLangByParam(TStringBuf lang) {
        return IsIn({"ru", "ua", "by", "kz", "kk", "uz"}, lang) ? LANG_RUS : LANG_ENG;
    }
}


TCaptchaRequest::TCaptchaRequest(const TEnv* env, const TRequest& request) {
    TCgiParameters postParams = GetPostParameters(request);

    auto getCgi = [&postParams, &request](TStringBuf key) {
        if (request.CgiParams.Has(key)) {
            return request.CgiParams.Get(key);
        }
        if (postParams.Has(key)) {
            return postParams.Get(key);
        }
        return TString();
    };

    TextResponse = getCgi("rep");
    Key = getCgi("key");
    Lang = GetLangByParam(getCgi("lang"));
    SiteKey = getCgi("sitekey");
    FailCheckbox = IsTrue(getCgi("test")) || request.Cookies.Has("YX_FAIL_CHECKBOX");

    try {
        if (postParams.Has("rdata")) {
            bool fingerprintOk = false;

            if (env && !env->HypocrisyBundle.Instances.empty()) {
                try {
                    const auto fingerprintData = NHypocrisy::TFingerprintData::Decrypt(
                        env->HypocrisyBundle.DescriptorKey,
                        postParams.Get("rdata")
                    );

                    if (fingerprintData) {
                        BrowserJsPrint.ParseFromJson(fingerprintData->Fingerprint);
                        fingerprintOk = true;
                    }
                } catch (...) {
                    EVLOG_MSG << CurrentExceptionMessage();
                }
            }

            if (!fingerprintOk && !ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
                // TODO: этот вызов остался для обратной совместимости, позже можно удалить
                ExtractFingerprint(request.UserAddr, postParams, &BrowserJsPrint);
            }
        }
    } catch (...) {
        EVLOG_MSG << EVLOG_ERROR << CurrentExceptionMessage();
    }
}

namespace {
    void FillField(const THashMap<TString, NJson::TJsonValue>& json, EBrowserJsPrintFeature feature, bool& prop) {
        if (const auto key = ANTIROBOT_DAEMON_CONFIG.BrowserJsPrintFeaturesMapping.FindPtr(feature)) {
            if (const auto value = json.FindPtr(*key)) {
                if (value->IsBoolean()) {
                    prop = value->GetBooleanSafe();
                } else if (value->IsString()) {
                    prop = value->GetStringSafe() == "true"_sb;
                }
            }
        }
    }

    void FillField(const THashMap<TString, NJson::TJsonValue>& json, EBrowserJsPrintFeature feature, i64& prop) {
        if (const auto key = ANTIROBOT_DAEMON_CONFIG.BrowserJsPrintFeaturesMapping.FindPtr(feature)) {
            if (const auto value = json.FindPtr(*key)) {
                if (value->IsInteger()) {
                    prop = static_cast<i64>(value->GetIntegerSafe());
                } else if (value->IsString()) {
                    TryFromString(value->GetStringSafe(), prop);
                }
            }
        }
    }

    void FillField(const THashMap<TString, NJson::TJsonValue>& json, EBrowserJsPrintFeature feature, TString& prop) {
        if (const auto key = ANTIROBOT_DAEMON_CONFIG.BrowserJsPrintFeaturesMapping.FindPtr(feature)) {
            if (const auto value = json.FindPtr(*key); value && value->IsString()) {
                prop = value->GetStringSafe();
            }
        }
    }

    void WriteField(NJsonWriter::TBuf& json, EBrowserJsPrintFeature feature, bool value) {
        json.WriteKey(ToString(feature)).WriteBool(value);
    }

    void WriteField(NJsonWriter::TBuf& json, EBrowserJsPrintFeature feature, i64 value) {
        json.WriteKey(ToString(feature)).WriteLongLong(value);
    }

    void WriteField(NJsonWriter::TBuf& json, EBrowserJsPrintFeature feature, const TString& value) {
        json.WriteKey(ToString(feature)).WriteString(value);
    }
}


void TCaptchaRequest::TBrowserJsPrint::ParseFromJsonString(const TString& jsonStr) {
    HasData = true;

    NJson::TJsonValue json;
    ValidJson = NJson::ReadJsonTree(jsonStr, &json);

    if (!ValidJson) {
        return;
    }

    ParseFromJson(json);
}

void TCaptchaRequest::TBrowserJsPrint::ParseFromJson(const NJson::TJsonValue& json) {
    HasData = true;
    ValidJson = json.IsMap();

    if (!ValidJson) {
        return;
    }

    const auto& map = json.GetMapSafe();

#define JSPRINT_FILL_FIELD(feature) FillField(map, EBrowserJsPrintFeature::feature, feature)

    JSPRINT_FILL_FIELD(BrowserNavigatorLanguage);
    JSPRINT_FILL_FIELD(BrowserNavigatorVendor);
    JSPRINT_FILL_FIELD(BrowserNavigatorProductSub);
    JSPRINT_FILL_FIELD(BrowserNavigatorLanguages);
    JSPRINT_FILL_FIELD(BrowserNavigatorProduct);
    JSPRINT_FILL_FIELD(BrowserPluginsPluginsLength);
    JSPRINT_FILL_FIELD(HeadlessNavigatorHasWebDriver);
    JSPRINT_FILL_FIELD(BrowserPluginsIsAdblockUsed);
    JSPRINT_FILL_FIELD(BrowserNavigatorIe9OrLower);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsChromium);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsTrident);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsDesktopSafari);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsChromium86OrNewer);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsEdgeHTML);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsGecko);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsWebKit);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsOpera);
    JSPRINT_FILL_FIELD(BrowserNavigatorIsBrave);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetEmptyEvalLength);
    JSPRINT_FILL_FIELD(BrowserFeaturesAvailableScreenResolution);
    JSPRINT_FILL_FIELD(BrowserFeaturesScreenResolution);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetSessionStorage);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetLocalStorage);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetColorDepth);
    JSPRINT_FILL_FIELD(BrowserFeaturesAreCookiesEnabled);
    JSPRINT_FILL_FIELD(BrowserNavigatorDeviceMemory);
    JSPRINT_FILL_FIELD(BrowserNavigatorCpuClass);
    JSPRINT_FILL_FIELD(BrowserNavigatorOscpu);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetHardwareConcurrency);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetErrorFF);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetIndexedDB);
    JSPRINT_FILL_FIELD(BrowserFeaturesOpenDatabase);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetTimezone);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetTimezoneOffset);
    JSPRINT_FILL_FIELD(BrowserFeaturesGetTouchSupport);
    JSPRINT_FILL_FIELD(BrowserImpervaBind);
    JSPRINT_FILL_FIELD(BrowserImpervaBuffer);
    JSPRINT_FILL_FIELD(BrowserImpervaChrome);
    JSPRINT_FILL_FIELD(BrowserImpervaDeviceMotionEvent);
    JSPRINT_FILL_FIELD(BrowserImpervaDeviceOrientationEvent);
    JSPRINT_FILL_FIELD(BrowserImpervaEmit);
    JSPRINT_FILL_FIELD(BrowserImpervaEventListener);
    JSPRINT_FILL_FIELD(BrowserImpervaInnerWidth);
    JSPRINT_FILL_FIELD(BrowserImpervaOuterWidth);
    JSPRINT_FILL_FIELD(BrowserImpervaPointerEvent);
    JSPRINT_FILL_FIELD(BrowserImpervaSpawn);
    JSPRINT_FILL_FIELD(BrowserImperva_TouchEvent);
    JSPRINT_FILL_FIELD(BrowserImpervaXDomainRequest);
    JSPRINT_FILL_FIELD(BrowserImpervaXMLHttpRequest);
    JSPRINT_FILL_FIELD(BrowserImpervaCallPhantom);
    JSPRINT_FILL_FIELD(BrowserImpervaActiveXObject);
    JSPRINT_FILL_FIELD(BrowserImpervaDocumentMode);
    JSPRINT_FILL_FIELD(BrowserImpervaWebstore);
    JSPRINT_FILL_FIELD(BrowserImpervaOnLine);
    JSPRINT_FILL_FIELD(BrowserImpervaInstallTrigger);
    JSPRINT_FILL_FIELD(BrowserImpervaHTMLElementConstructor);
    JSPRINT_FILL_FIELD(BrowserImpervaRTCPeerConnection);
    JSPRINT_FILL_FIELD(BrowserImpervaMozInnerScreenY);
    JSPRINT_FILL_FIELD(BrowserImpervaVibrate);
    JSPRINT_FILL_FIELD(BrowserImpervaGetBattery);
    JSPRINT_FILL_FIELD(BrowserImpervaForEach);
    JSPRINT_FILL_FIELD(BrowserImpervaFileReader);
    JSPRINT_FILL_FIELD(HeadlessNavigatorIframeChrome);
    JSPRINT_FILL_FIELD(HeadlessImpervaDriver);
    JSPRINT_FILL_FIELD(HeadlessImpervaMagicString);
    JSPRINT_FILL_FIELD(HeadlessImpervaSelenium);
    JSPRINT_FILL_FIELD(HeadlessImpervaWebdriver);
    JSPRINT_FILL_FIELD(HeadlessImpervaXPathResult);
    JSPRINT_FILL_FIELD(HeadlessFingerprintJsProRunBotDetection);
    JSPRINT_FILL_FIELD(HeadlessAudiocontext);
    JSPRINT_FILL_FIELD(HeadlessBadgingApi);
    JSPRINT_FILL_FIELD(HeadlessCanvasToBlob);
    JSPRINT_FILL_FIELD(HeadlessIntlNumberFormat);
    JSPRINT_FILL_FIELD(HeadlessIntlDatetimeFormat);
    JSPRINT_FILL_FIELD(HeadlessNavigatorLevel);
    JSPRINT_FILL_FIELD(HeadlessNotificationToString);
    JSPRINT_FILL_FIELD(HeadlessSandboxedIframe);
    JSPRINT_FILL_FIELD(HeadlessSensorsAccelerometer);
    JSPRINT_FILL_FIELD(HeadlessSpeechVoices);
    JSPRINT_FILL_FIELD(HeadlessWidevineDrm);
    JSPRINT_FILL_FIELD(HeadlessPermissionGeolocation);
    JSPRINT_FILL_FIELD(HeadlessPermissionPush);
    JSPRINT_FILL_FIELD(HeadlessPermissionMidi);
    JSPRINT_FILL_FIELD(HeadlessPermissionCamera);
    JSPRINT_FILL_FIELD(HeadlessPermissionMicrophone);
    JSPRINT_FILL_FIELD(HeadlessPermissionSpeakerSelection);
    JSPRINT_FILL_FIELD(HeadlessPermissionDeviceInfo);
    JSPRINT_FILL_FIELD(HeadlessPermissionBackgroundFetch);
    JSPRINT_FILL_FIELD(HeadlessPermissionBackgroundSync);
    JSPRINT_FILL_FIELD(HeadlessPermissionBluetooth);
    JSPRINT_FILL_FIELD(HeadlessPermissionPersistentStorage);
    JSPRINT_FILL_FIELD(HeadlessPermissionAmbientLightSensor);
    JSPRINT_FILL_FIELD(HeadlessPermissionGyroscope);
    JSPRINT_FILL_FIELD(HeadlessPermissionMagnetometer);
    JSPRINT_FILL_FIELD(HeadlessPermissionClipboardRead);
    JSPRINT_FILL_FIELD(HeadlessPermissionClipboardWrite);
    JSPRINT_FILL_FIELD(HeadlessPermissionDisplayCapture);
    JSPRINT_FILL_FIELD(HeadlessPermissionNfc);
    JSPRINT_FILL_FIELD(HeadlessPermissionNotifications);
    JSPRINT_FILL_FIELD(EnvFontsAvailable);
    JSPRINT_FILL_FIELD(EnvGetCanvasFingerprint);
    JSPRINT_FILL_FIELD(Version);

#undef JSPRINT_FILL_FIELD
}

void TCaptchaRequest::TBrowserJsPrint::WriteToJson(NJsonWriter::TBuf& json) const {
    json.BeginObject();

#define JSPRINT_WRITE_FIELD(feature) WriteField(json, EBrowserJsPrintFeature::feature, feature)

    JSPRINT_WRITE_FIELD(ValidJson);
    JSPRINT_WRITE_FIELD(HasData);

    JSPRINT_WRITE_FIELD(BrowserNavigatorLanguage);
    JSPRINT_WRITE_FIELD(BrowserNavigatorVendor);
    JSPRINT_WRITE_FIELD(BrowserNavigatorProductSub);
    JSPRINT_WRITE_FIELD(BrowserNavigatorLanguages);
    JSPRINT_WRITE_FIELD(BrowserNavigatorProduct);
    JSPRINT_WRITE_FIELD(BrowserPluginsPluginsLength);
    JSPRINT_WRITE_FIELD(HeadlessNavigatorHasWebDriver);
    JSPRINT_WRITE_FIELD(BrowserPluginsIsAdblockUsed);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIe9OrLower);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsChromium);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsTrident);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsDesktopSafari);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsChromium86OrNewer);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsEdgeHTML);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsGecko);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsWebKit);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsOpera);
    JSPRINT_WRITE_FIELD(BrowserNavigatorIsBrave);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetEmptyEvalLength);
    JSPRINT_WRITE_FIELD(BrowserFeaturesAvailableScreenResolution);
    JSPRINT_WRITE_FIELD(BrowserFeaturesScreenResolution);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetSessionStorage);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetLocalStorage);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetColorDepth);
    JSPRINT_WRITE_FIELD(BrowserFeaturesAreCookiesEnabled);
    JSPRINT_WRITE_FIELD(BrowserNavigatorDeviceMemory);
    JSPRINT_WRITE_FIELD(BrowserNavigatorCpuClass);
    JSPRINT_WRITE_FIELD(BrowserNavigatorOscpu);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetHardwareConcurrency);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetErrorFF);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetIndexedDB);
    JSPRINT_WRITE_FIELD(BrowserFeaturesOpenDatabase);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetTimezone);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetTimezoneOffset);
    JSPRINT_WRITE_FIELD(BrowserFeaturesGetTouchSupport);
    JSPRINT_WRITE_FIELD(BrowserImpervaBind);
    JSPRINT_WRITE_FIELD(BrowserImpervaBuffer);
    JSPRINT_WRITE_FIELD(BrowserImpervaChrome);
    JSPRINT_WRITE_FIELD(BrowserImpervaDeviceMotionEvent);
    JSPRINT_WRITE_FIELD(BrowserImpervaDeviceOrientationEvent);
    JSPRINT_WRITE_FIELD(BrowserImpervaEmit);
    JSPRINT_WRITE_FIELD(BrowserImpervaEventListener);
    JSPRINT_WRITE_FIELD(BrowserImpervaInnerWidth);
    JSPRINT_WRITE_FIELD(BrowserImpervaOuterWidth);
    JSPRINT_WRITE_FIELD(BrowserImpervaPointerEvent);
    JSPRINT_WRITE_FIELD(BrowserImpervaSpawn);
    JSPRINT_WRITE_FIELD(BrowserImperva_TouchEvent);
    JSPRINT_WRITE_FIELD(BrowserImpervaXDomainRequest);
    JSPRINT_WRITE_FIELD(BrowserImpervaXMLHttpRequest);
    JSPRINT_WRITE_FIELD(BrowserImpervaCallPhantom);
    JSPRINT_WRITE_FIELD(BrowserImpervaActiveXObject);
    JSPRINT_WRITE_FIELD(BrowserImpervaDocumentMode);
    JSPRINT_WRITE_FIELD(BrowserImpervaWebstore);
    JSPRINT_WRITE_FIELD(BrowserImpervaOnLine);
    JSPRINT_WRITE_FIELD(BrowserImpervaInstallTrigger);
    JSPRINT_WRITE_FIELD(BrowserImpervaHTMLElementConstructor);
    JSPRINT_WRITE_FIELD(BrowserImpervaRTCPeerConnection);
    JSPRINT_WRITE_FIELD(BrowserImpervaMozInnerScreenY);
    JSPRINT_WRITE_FIELD(BrowserImpervaVibrate);
    JSPRINT_WRITE_FIELD(BrowserImpervaGetBattery);
    JSPRINT_WRITE_FIELD(BrowserImpervaForEach);
    JSPRINT_WRITE_FIELD(BrowserImpervaFileReader);
    JSPRINT_WRITE_FIELD(HeadlessNavigatorIframeChrome);
    JSPRINT_WRITE_FIELD(HeadlessImpervaDriver);
    JSPRINT_WRITE_FIELD(HeadlessImpervaMagicString);
    JSPRINT_WRITE_FIELD(HeadlessImpervaSelenium);
    JSPRINT_WRITE_FIELD(HeadlessImpervaWebdriver);
    JSPRINT_WRITE_FIELD(HeadlessImpervaXPathResult);
    JSPRINT_WRITE_FIELD(HeadlessFingerprintJsProRunBotDetection);
    JSPRINT_WRITE_FIELD(HeadlessAudiocontext);
    JSPRINT_WRITE_FIELD(HeadlessBadgingApi);
    JSPRINT_WRITE_FIELD(HeadlessCanvasToBlob);
    JSPRINT_WRITE_FIELD(HeadlessIntlNumberFormat);
    JSPRINT_WRITE_FIELD(HeadlessIntlDatetimeFormat);
    JSPRINT_WRITE_FIELD(HeadlessNavigatorLevel);
    JSPRINT_WRITE_FIELD(HeadlessNotificationToString);
    JSPRINT_WRITE_FIELD(HeadlessSandboxedIframe);
    JSPRINT_WRITE_FIELD(HeadlessSensorsAccelerometer);
    JSPRINT_WRITE_FIELD(HeadlessSpeechVoices);
    JSPRINT_WRITE_FIELD(HeadlessWidevineDrm);
    JSPRINT_WRITE_FIELD(HeadlessPermissionGeolocation);
    JSPRINT_WRITE_FIELD(HeadlessPermissionPush);
    JSPRINT_WRITE_FIELD(HeadlessPermissionMidi);
    JSPRINT_WRITE_FIELD(HeadlessPermissionCamera);
    JSPRINT_WRITE_FIELD(HeadlessPermissionMicrophone);
    JSPRINT_WRITE_FIELD(HeadlessPermissionSpeakerSelection);
    JSPRINT_WRITE_FIELD(HeadlessPermissionDeviceInfo);
    JSPRINT_WRITE_FIELD(HeadlessPermissionBackgroundFetch);
    JSPRINT_WRITE_FIELD(HeadlessPermissionBackgroundSync);
    JSPRINT_WRITE_FIELD(HeadlessPermissionBluetooth);
    JSPRINT_WRITE_FIELD(HeadlessPermissionPersistentStorage);
    JSPRINT_WRITE_FIELD(HeadlessPermissionAmbientLightSensor);
    JSPRINT_WRITE_FIELD(HeadlessPermissionGyroscope);
    JSPRINT_WRITE_FIELD(HeadlessPermissionMagnetometer);
    JSPRINT_WRITE_FIELD(HeadlessPermissionClipboardRead);
    JSPRINT_WRITE_FIELD(HeadlessPermissionClipboardWrite);
    JSPRINT_WRITE_FIELD(HeadlessPermissionDisplayCapture);
    JSPRINT_WRITE_FIELD(HeadlessPermissionNfc);
    JSPRINT_WRITE_FIELD(HeadlessPermissionNotifications);
    JSPRINT_WRITE_FIELD(EnvFontsAvailable);
    JSPRINT_WRITE_FIELD(EnvGetCanvasFingerprint);
    JSPRINT_WRITE_FIELD(Version);

#undef JSPRINT_WRITE_FIELD

    json.EndObject();
}


} // namespace NAntiRobot
