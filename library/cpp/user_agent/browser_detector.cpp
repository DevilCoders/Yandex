#include "browser_detector.h"

#include <metrika/uatraits/include/uatraits/detector.hpp>

#include <util/generic/cast.h> // SafeIntegerCast
#include <util/generic/yexception.h>

namespace NUserAgent {
    TBrowserDetector::TBrowserDetector()
        : Detector(nullptr)
    {
        // From https://svn.yandex.ru/websvn/wsvn/conv/trunk/metrica/src/db_dumps/Metrica/UserAgent.sql?rev=36852
        // Regexps must have at most one capturing group (returned as version).
        BrowserRegexps.push_back(TRegexpPair("AndroidBrowser", // weight = -25
                                             TRegExMatch("^Mozilla/(?:[0-9\\.]+)? \\([A-z]+; U; Android (\\d+(?:\\.\\d+)?)(?:\\.\\d+)?.*\\) AppleWebKit/(?:[0-9\\.]+)? \\(KHTML, like Gecko\\) Version/(?:[0-9\\.]+)?(?: Mobile)? Safari/(?:[0-9\\.]+)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("YandexBrowser", // weight = -24
                                             TRegExMatch("YaBrowser[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("LGPhantomBrowser", // weight = -23
                                             TRegExMatch("Phantom(/V?\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("NokiaSeries60Browser", // weight = -22
                                             TRegExMatch("S(?:eries)?60(/\\d+(?:\\.\\d{1,2})?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Skyfire", // weight = -21
                                             TRegExMatch("Skyfire(/\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("TeaShark", // weight = -20
                                             TRegExMatch("TeaShark(/\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("InternetMailRu", // weight = -19
                                             TRegExMatch("Chrome[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?.*? MRCHROME", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("YandexInternet", // weight = -18
                                             TRegExMatch("Chrome[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?.*? (?:YI|YE)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("QupZilla", // weight = -17
                                             TRegExMatch("QupZilla(/\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("OperaTablet", // weight = -16
                                             TRegExMatch("Opera Tablet.*?Version/(\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("iChromy", // weight = -15
                                             TRegExMatch("(?:iChromy|DiigoBrowser) .*?Version/(\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Fennec", // weight = -13
                                             TRegExMatch("Fennec[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Blazer", // weight = -12
                                             TRegExMatch("Blazer[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("AdobeAIR", // weight = -11
                                             TRegExMatch("Adobe ?AIR[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("MoblinWebBrowser", // weight = -10
                                             TRegExMatch("Moblin ?Web ?Browser[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Novarra", // weight = -9
                                             TRegExMatch("Novarra(?:-Vision)?[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("OperaMobile", // weight = -8
                                             TRegExMatch("Opera[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?.*?Mobi", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("NetCaptor", // weight = -7
                                             TRegExMatch("NetCaptor[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("MyIE2", // weight = -6
                                             TRegExMatch("MSIE\\s(\\d+(?:\\.[\\dA-Za-z]{1,2})?)?.*[^a-zA-z0-9/]MyIE2", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("MyIE", // weight = -5
                                             TRegExMatch("MSIE\\s(\\d+(?:\\.[\\dA-Za-z]{1,2})?)?.*[^a-zA-z0-9/]MyIE[^a-zA-z0-9]", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Maxthon", // weight = -4
                                             TRegExMatch("MSIE\\s(\\d+(?:\\.[\\dA-Za-z]{1,2})?)?.*[^a-zA-z0-9/]Maxthon", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("AvantBrowser", // weight = -3
                                             TRegExMatch("MSIE\\s(\\d+(?:\\.[\\dA-Za-z]{1,2})?)?.*[^a-zA-z0-9/]Avant Browser", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Flock", // weight = -2
                                             TRegExMatch("Flock[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Epiphany", // weight = -1
                                             TRegExMatch("Epiphany[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("GoogleChrome", // weight = 1
                                             TRegExMatch("(?:Chrome|CriOS)[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("OperaMini", // weight = 2
                                             TRegExMatch("Opera Mini[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Opera", // weight = 3
                                             TRegExMatch("Opera(?:.*?Version)?[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Firefox", // weight = 4
                                             TRegExMatch("(?:Firefox|Shiretoko|Pescadero|Santa ?Cruz|Lucia|Oceano|Naples|Glendale|Indio|Royal ?Oak|One ?Tree ?Hill|Phoenix|Deer ?Park|Bon ?Echo|Gran ?Paradiso)[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("MobileSafari", // weight = 4
                                             TRegExMatch("(?:(?:Version/(\\d+(?:\\.\\d+)?)(?:\\.\\d+)*)?[^a-zA-z0-9/]Mobile.*?Safari[/ ]|AppleWebKit/[\\d\\.]+ \\(KHTML, like Gecko\\) Mobile)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("rekonq", // weight = 4
                                             TRegExMatch("rekonq Safari[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Safari", // weight = 5
                                             TRegExMatch("(?:Version/(\\d+(?:\\.\\d+)?)(?:\\.\\d+)*)?[^a-zA-z0-9/]Safari[/ ]", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("MSIEMobile", // weight = 5
                                             TRegExMatch("IE ?Mobile[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("MSIE", // weight = 6
                                             TRegExMatch("MSIE\\s(\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Minefield", // weight = 7
                                             TRegExMatch("Minefield[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("SeaMonkey", // weight = 8
                                             TRegExMatch("SeaMonkey[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Netscape", // weight = 9
                                             TRegExMatch("Netscape[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("K-Meleon", // weight = 10
                                             TRegExMatch("K-Meleon[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Iceweasel", // weight = 11
                                             TRegExMatch("Iceweasel[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Konqueror", // weight = 12
                                             TRegExMatch("Konqueror[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("UCWEB", // weight = 14
                                             TRegExMatch("UC(?:WEB| Browser)[/ ]?(\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("NetFront", // weight = 15
                                             TRegExMatch("NetFront[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Songbird", // weight = 17
                                             TRegExMatch("Songbird[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("MAUIWAPBrowser", // weight = 18
                                             TRegExMatch("MAUI[ _]WAP[ _]Browser", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("PlayStation3", // weight = 19
                                             TRegExMatch("PLAYSTATION 3(; \\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Iceape", // weight = 20
                                             TRegExMatch("Iceape[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Camino", // weight = 21
                                             TRegExMatch("Camino[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("SEMC-Browser", // weight = 22
                                             TRegExMatch("SEMC-Browser[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("UP.Browser", // weight = 23
                                             TRegExMatch("UP\\.Browser[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("NCSAMosaic", // weight = 24
                                             TRegExMatch("NCSA Mosaic[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Galeon", // weight = 25
                                             TRegExMatch("Galeon[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("ObigoBrowser", // weight = 26
                                             TRegExMatch("Obigo[ \\-/;]", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Mozad", // weight = 27
                                             TRegExMatch("Mozad[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Conkeror", // weight = 27
                                             TRegExMatch("Conkeror[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Arora", // weight = 28
                                             TRegExMatch("Arora[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("NetSurf", // weight = 28
                                             TRegExMatch("NetSurf[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Lynx", // weight = 29
                                             TRegExMatch("Lynx[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Dillo", // weight = 29
                                             TRegExMatch("Dillo[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Links", // weight = 30
                                             TRegExMatch("Links ?\\((\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Thunderbird", // weight = 30
                                             TRegExMatch("Thunderbird[/ ](\\d+(?:\\.\\d+)?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("NokiaBrowserNG", // weight = 31
                                             TRegExMatch("BrowserNG(/\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("OviBrowser", // weight = 32
                                             TRegExMatch("OviBrowser(/\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("Uzbl", // weight = 33
                                             TRegExMatch("Uzbl", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("SamsungDolfin", // weight = 34
                                             TRegExMatch("Dolfin(/\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("SamsungJasmine", // weight = 35
                                             TRegExMatch("Jasmine(/\\d+(?:\\.\\d+)?)", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("TridentOther", // weight = 1001
                                             TRegExMatch("Trident[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("PrestoOther", // weight = 1003
                                             TRegExMatch("Presto[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("WebKitOther", // weight = 1004
                                             TRegExMatch("(?:Apple)?WebKit[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("KHTMLOther", // weight = 1005
                                             TRegExMatch("KHTML[/ ](\\d+(?:\\.[\\dA-Za-z]{1,2})?)?", REG_EXTENDED)));
        BrowserRegexps.push_back(TRegexpPair("GeckoOther", // weight = 1010
                                             TRegExMatch("(?:rv:(\\d+(?:\\.[\\dA-Za-z]{1,2}))(?:\\.[\\dA-Za-z]{1,2})*\\) )?Gecko(?:$|[^a-zA-z0-9])", REG_EXTENDED)));

        // unknown browser
        BrowserRegexps.push_back(TRegexpPair("Unknown", TRegExMatch(".?", REG_EXTENDED)));
    }

    TBrowserDetector& TBrowserDetector::Instance() {
        static TBrowserDetector instance;
        return instance;
    }

    const TString& TBrowserDetector::GetBrowser(const TString& userAgent, TString* browserVersion) {
        //uatraits detection
        if (Detector != nullptr) {
            auto result = Detector->detect(userAgent.data());
            const auto it = result.find("BrowserName");
            if (it == result.end()) {
                ythrow yexception() << "No 'BrowserName' in uatraits. This should never happen.";
            }
            if (browserVersion && (result.find("BrowserVersion") != result.end())) {
                *browserVersion = TString(result["BrowserVersion"]);
            }
            Browser = result["BrowserName"];
            return Browser;
        }

        //no uatraits
        for (size_t i = 0; i < BrowserRegexps.size(); ++i) {
            regmatch_t matchGroups[2];
            if (BrowserRegexps[i].second.Exec(userAgent.c_str(), matchGroups, 0, 2) == 0) {
                if (browserVersion) {
                    *browserVersion = LegacySubstr(userAgent, matchGroups[1].rm_so,
                                              matchGroups[1].rm_eo - matchGroups[1].rm_so);
                }
                return BrowserRegexps[i].first;
            }
        }
        ythrow yexception() << "No regexps matched. This should never happen.";
    }

    const TString& TBrowserDetector::GetOS(const TString& userAgent, TString* osVersion) {
        //uatraits detection
        if (Detector != nullptr) {
            auto result = Detector->detect(userAgent.data());
            const auto itName = result.find("OSFamily");
            if (itName != result.end()) {
                if (osVersion && (result.find("OSVersion") != result.end())) {
                    *osVersion = TString(result["OSVersion"]);
                }
                OS = result["OSFamily"];
            }
        }

        return OS;
    }

    uatraits::detector::result_type TBrowserDetector::GetUatraitsData(const TString& userAgent) {
        if (Detector == nullptr) {
            return uatraits::detector::result_type();
        } else {
            return Detector->detect(userAgent.data());
        }
    }

    bool TBrowserDetector::InitializeUatraits(const char* uaconfigPath) {
        try {
            Detector.Reset(new uatraits::detector(uaconfigPath));
        } catch (...) {
            return false;
        }
        return true;
    }

    bool TBrowserDetector::InitializeUatraitsFromMemory(const TStringBuf uaconfigData) {
        try {
            Detector.Reset(new uatraits::detector(uaconfigData.data(), SafeIntegerCast<int>(uaconfigData.size())));
        } catch (...) {
            return false;
        }
        return true;
    }

}
