#pragma once

#include <library/cpp/json/writer/json.h>
#include <library/cpp/langs/langs.h>

namespace NAntiRobot {
    struct TEnv;
    class TRequest;

    enum class EBrowserJsPrintFeature : ui32 {
        ValidJson /* "valid_json" */,
        HasData /* "has_data" */,

        BrowserNavigatorLanguage /* "browser_navigator_language" */,
        BrowserNavigatorVendor /* "browser_navigator_vendor" */,
        BrowserNavigatorProductSub /* "browser_navigator_productSub" */,
        BrowserNavigatorLanguages /* "browser_navigator_languages" */,
        BrowserNavigatorProduct /* "browser_navigator_product" */,
        BrowserPluginsPluginsLength /* "browser_plugins_pluginsLength" */,
        HeadlessNavigatorHasWebDriver /* "headless_navigator_hasWebDriver" */,
        BrowserPluginsIsAdblockUsed /* "browser_plugins_isAdblockUsed" */,
        BrowserNavigatorIe9OrLower /* "browser_navigator_ie9OrLower" */,
        BrowserNavigatorIsChromium /* "browser_navigator_isChromium" */,
        BrowserNavigatorIsTrident /* "browser_navigator_isTrident" */,
        BrowserNavigatorIsDesktopSafari /* "browser_navigator_isDesktopSafari" */,
        BrowserNavigatorIsChromium86OrNewer /* "browser_navigator_isChromium86OrNewer" */,
        BrowserNavigatorIsEdgeHTML /* "browser_navigator_isEdgeHTML" */,
        BrowserNavigatorIsGecko /* "browser_navigator_isGecko" */,
        BrowserNavigatorIsWebKit /* "browser_navigator_isWebKit" */,
        BrowserNavigatorIsOpera /* "browser_navigator_isOpera" */,
        BrowserNavigatorIsBrave /* "browser_navigator_isBrave" */,
        BrowserFeaturesGetEmptyEvalLength /* "browser_features_getEmptyEvalLength" */,
        BrowserFeaturesAvailableScreenResolution /* "browser_features_availableScreenResolution" */,
        BrowserFeaturesScreenResolution /* "browser_features_screenResolution" */,
        BrowserFeaturesGetSessionStorage /* "browser_features_getSessionStorage" */,
        BrowserFeaturesGetLocalStorage /* "browser_features_getLocalStorage" */,
        BrowserFeaturesGetColorDepth /* "browser_features_getColorDepth" */,
        BrowserFeaturesAreCookiesEnabled /* "browser_features_areCookiesEnabled" */,
        BrowserNavigatorDeviceMemory /* "browser_navigator_deviceMemory" */,
        BrowserNavigatorCpuClass /* "browser_navigator_cpuClass" */,
        BrowserNavigatorOscpu /* "browser_navigator_oscpu" */,
        BrowserFeaturesGetHardwareConcurrency /* "browser_features_getHardwareConcurrency" */,
        BrowserFeaturesGetErrorFF /* "browser_features_getErrorFF" */,
        BrowserFeaturesGetIndexedDB /* "browser_features_getIndexedDB" */,
        BrowserFeaturesOpenDatabase /* "browser_features_openDatabase" */,
        BrowserFeaturesGetTimezone /* "browser_features_getTimezone" */,
        BrowserFeaturesGetTimezoneOffset /* "browser_features_getTimezoneOffset" */,
        BrowserFeaturesGetTouchSupport /* "browser_features_getTouchSupport" */,
        BrowserImpervaBind /* "browser_imperva_bind" */,
        BrowserImpervaBuffer /* "browser_imperva_buffer" */,
        BrowserImpervaChrome /* "browser_imperva_chrome" */,
        BrowserImpervaDeviceMotionEvent /* "browser_imperva_deviceMotionEvent" */,
        BrowserImpervaDeviceOrientationEvent /* "browser_imperva_deviceOrientationEvent" */,
        BrowserImpervaEmit /* "browser_imperva_emit" */,
        BrowserImpervaEventListener /* "browser_imperva_eventListener" */,
        BrowserImpervaInnerWidth /* "browser_imperva_innerWidth" */,
        BrowserImpervaOuterWidth /* "browser_imperva_outerWidth" */,
        BrowserImpervaPointerEvent /* "browser_imperva_pointerEvent" */,
        BrowserImpervaSpawn /* "browser_imperva_spawn" */,
        BrowserImperva_TouchEvent /* "browser_imperva_TouchEvent" */,
        BrowserImpervaXDomainRequest /* "browser_imperva_xDomainRequest" */,
        BrowserImpervaXMLHttpRequest /* "browser_imperva_xMLHttpRequest" */,
        BrowserImpervaCallPhantom /* "browser_imperva_callPhantom" */,
        BrowserImpervaActiveXObject /* "browser_imperva_activeXObject" */,
        BrowserImpervaDocumentMode /* "browser_imperva_documentMode" */,
        BrowserImpervaWebstore /* "browser_imperva_webstore" */,
        BrowserImpervaOnLine /* "browser_imperva_onLine" */,
        BrowserImpervaInstallTrigger /* "browser_imperva_installTrigger" */,
        BrowserImpervaHTMLElementConstructor /* "browser_imperva_hTMLElementConstructor" */,
        BrowserImpervaRTCPeerConnection /* "browser_imperva_rTCPeerConnection" */,
        BrowserImpervaMozInnerScreenY /* "browser_imperva_mozInnerScreenY" */,
        BrowserImpervaVibrate /* "browser_imperva_vibrate" */,
        BrowserImpervaGetBattery /* "browser_imperva_getBattery" */,
        BrowserImpervaForEach /* "browser_imperva_forEach" */,
        BrowserImpervaFileReader /* "browser_imperva_fileReader" */,
        HeadlessNavigatorIframeChrome /* "headless_navigator_iframeChrome" */,
        HeadlessImpervaDriver /* "headless_imperva_driver" */,
        HeadlessImpervaMagicString /* "headless_imperva_magicString" */,
        HeadlessImpervaSelenium /* "headless_imperva_selenium" */,
        HeadlessImpervaWebdriver /* "headless_imperva_webdriver" */,
        HeadlessImpervaXPathResult /* "headless_imperva_xPathResult" */,
        HeadlessFingerprintJsProRunBotDetection /* "headless_fingerprintJsPro_runBotDetection" */,
        HeadlessAudiocontext /* "headless_audiocontext" */,
        HeadlessBadgingApi /* "headless_badgingApi" */,
        HeadlessCanvasToBlob /* "headless_canvasToBlob" */,
        HeadlessIntlNumberFormat /* "headless_intlNumberFormat" */,
        HeadlessIntlDatetimeFormat /* "headless_intlDatetimeFormat" */,
        HeadlessNavigatorLevel /* "headless_navigatorLevel" */,
        HeadlessNotificationToString /* "headless_notificationToString" */,
        HeadlessSandboxedIframe /* "headless_sandboxedIframe" */,
        HeadlessSensorsAccelerometer /* "headless_sensorsAccelerometer" */,
        HeadlessSpeechVoices /* "headless_speechVoices" */,
        HeadlessWidevineDrm /* "headless_widevineDrm" */,
        HeadlessPermissionGeolocation /* "headless_permissionGeolocation" */,
        HeadlessPermissionPush /* "headless_permissionPush" */,
        HeadlessPermissionMidi /* "headless_permissionMidi" */,
        HeadlessPermissionCamera /* "headless_permissionCamera" */,
        HeadlessPermissionMicrophone /* "headless_permissionMicrophone" */,
        HeadlessPermissionSpeakerSelection /* "headless_permissionSpeakerSelection" */,
        HeadlessPermissionDeviceInfo /* "headless_permissionDeviceInfo" */,
        HeadlessPermissionBackgroundFetch /* "headless_permissionBackgroundFetch" */,
        HeadlessPermissionBackgroundSync /* "headless_permissionBackgroundSync" */,
        HeadlessPermissionBluetooth /* "headless_permissionBluetooth" */,
        HeadlessPermissionPersistentStorage /* "headless_permissionPersistentStorage" */,
        HeadlessPermissionAmbientLightSensor /* "headless_permissionAmbientLightSensor" */,
        HeadlessPermissionGyroscope /* "headless_permissionGyroscope" */,
        HeadlessPermissionMagnetometer /* "headless_permissionMagnetometer" */,
        HeadlessPermissionClipboardRead /* "headless_permissionClipboardRead" */,
        HeadlessPermissionClipboardWrite /* "headless_permissionClipboardWrite" */,
        HeadlessPermissionDisplayCapture /* "headless_permissionDisplayCapture" */,
        HeadlessPermissionNfc /* "headless_permissionNfc" */,
        HeadlessPermissionNotifications /* "headless_permissionNotifications" */,
        EnvFontsAvailable /* "env_fontsAvailable" */,
        EnvGetCanvasFingerprint /* "env_getCanvasFingerprint" */,
        Version /* "version" */,

        Count
    };

    struct TCaptchaRequest {
        struct TBrowserJsPrint {
            bool HasData = false;
            bool ValidJson = false;

            TString BrowserNavigatorLanguage;
            TString BrowserNavigatorVendor;
            TString BrowserNavigatorProductSub;
            TString BrowserNavigatorLanguages;
            TString BrowserNavigatorProduct;
            i64 BrowserPluginsPluginsLength = 0;
            bool HeadlessNavigatorHasWebDriver = false;
            bool BrowserPluginsIsAdblockUsed = false;
            bool BrowserNavigatorIe9OrLower = false;
            bool BrowserNavigatorIsChromium = false;
            bool BrowserNavigatorIsTrident = false;
            bool BrowserNavigatorIsDesktopSafari = false;
            bool BrowserNavigatorIsChromium86OrNewer = false;
            bool BrowserNavigatorIsEdgeHTML = false;
            bool BrowserNavigatorIsGecko = false;
            bool BrowserNavigatorIsWebKit = false;
            bool BrowserNavigatorIsOpera = false;
            bool BrowserNavigatorIsBrave = false;
            i64 BrowserFeaturesGetEmptyEvalLength = 0;
            TString BrowserFeaturesAvailableScreenResolution;
            TString BrowserFeaturesScreenResolution;
            bool BrowserFeaturesGetSessionStorage = false;
            bool BrowserFeaturesGetLocalStorage = false;
            i64 BrowserFeaturesGetColorDepth = 0;
            bool BrowserFeaturesAreCookiesEnabled = false;
            i64 BrowserNavigatorDeviceMemory = 0;
            TString BrowserNavigatorCpuClass;
            TString BrowserNavigatorOscpu;
            i64 BrowserFeaturesGetHardwareConcurrency = 0;
            bool BrowserFeaturesGetErrorFF = false;
            bool BrowserFeaturesGetIndexedDB = false;
            bool BrowserFeaturesOpenDatabase = false;
            TString BrowserFeaturesGetTimezone;
            i64 BrowserFeaturesGetTimezoneOffset = 0;
            TString BrowserFeaturesGetTouchSupport;
            bool BrowserImpervaBind = false;
            bool BrowserImpervaBuffer = false;
            bool BrowserImpervaChrome = false;
            bool BrowserImpervaDeviceMotionEvent = false;
            bool BrowserImpervaDeviceOrientationEvent = false;
            bool BrowserImpervaEmit = false;
            bool BrowserImpervaEventListener = false;
            bool BrowserImpervaInnerWidth = false;
            bool BrowserImpervaOuterWidth = false;
            bool BrowserImpervaPointerEvent = false;
            bool BrowserImpervaSpawn = false;
            bool BrowserImperva_TouchEvent = false;
            bool BrowserImpervaXDomainRequest = false;
            bool BrowserImpervaXMLHttpRequest = false;
            bool BrowserImpervaCallPhantom = false;
            bool BrowserImpervaActiveXObject = false;
            bool BrowserImpervaDocumentMode = false;
            bool BrowserImpervaWebstore = false;
            bool BrowserImpervaOnLine = false;
            bool BrowserImpervaInstallTrigger = false;
            bool BrowserImpervaHTMLElementConstructor = false;
            bool BrowserImpervaRTCPeerConnection = false;
            bool BrowserImpervaMozInnerScreenY = false;
            bool BrowserImpervaVibrate = false;
            bool BrowserImpervaGetBattery = false;
            bool BrowserImpervaForEach = false;
            bool BrowserImpervaFileReader = false;
            bool HeadlessNavigatorIframeChrome = false;
            bool HeadlessImpervaDriver = false;
            bool HeadlessImpervaMagicString = false;
            bool HeadlessImpervaSelenium = false;
            bool HeadlessImpervaWebdriver = false;
            bool HeadlessImpervaXPathResult = false;
            bool HeadlessFingerprintJsProRunBotDetection = false;
            TString HeadlessAudiocontext;
            bool HeadlessBadgingApi = false;
            bool HeadlessCanvasToBlob = false;
            bool HeadlessIntlNumberFormat = false;
            bool HeadlessIntlDatetimeFormat = false;
            bool HeadlessNavigatorLevel = false;
            bool HeadlessNotificationToString = false;
            bool HeadlessSandboxedIframe = false;
            TString HeadlessSensorsAccelerometer;
            TString HeadlessSpeechVoices;
            bool HeadlessWidevineDrm = false;
            TString HeadlessPermissionGeolocation;
            TString HeadlessPermissionPush;
            TString HeadlessPermissionMidi;
            TString HeadlessPermissionCamera;
            TString HeadlessPermissionMicrophone;
            TString HeadlessPermissionSpeakerSelection;
            TString HeadlessPermissionDeviceInfo;
            TString HeadlessPermissionBackgroundFetch;
            TString HeadlessPermissionBackgroundSync;
            TString HeadlessPermissionBluetooth;
            TString HeadlessPermissionPersistentStorage;
            TString HeadlessPermissionAmbientLightSensor;
            TString HeadlessPermissionGyroscope;
            TString HeadlessPermissionMagnetometer;
            TString HeadlessPermissionClipboardRead;
            TString HeadlessPermissionClipboardWrite;
            TString HeadlessPermissionDisplayCapture;
            TString HeadlessPermissionNfc;
            TString HeadlessPermissionNotifications;
            TString EnvFontsAvailable;
            TString EnvGetCanvasFingerprint;
            TString Version;

            void ParseFromJsonString(const TString& jsonStr);
            void ParseFromJson(const NJson::TJsonValue& json);
            void WriteToJson(NJsonWriter::TBuf& json) const;
        };

        TString TextResponse;
        TString Key;
        TString SiteKey;
        ELanguage Lang;
        TBrowserJsPrint BrowserJsPrint;
        bool FailCheckbox = false;

        TCaptchaRequest() = default;
        explicit TCaptchaRequest(const TEnv* env, const TRequest& request);
    };
}
