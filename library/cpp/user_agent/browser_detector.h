#pragma once

#include <library/cpp/regex/pcre/regexp.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/noncopyable.h>
#include <utility>
#include <map>
#include <string>

namespace uatraits {
    class detector;
}

namespace NUserAgent {
    class TBrowserDetector: public TNonCopyable {
    private:
        using TRegexpPair = std::pair<TString, TRegExMatch>;
        TVector<TRegexpPair> BrowserRegexps;
        THolder<uatraits::detector> Detector;
        TString Browser;
        TString OS;

    private:
        TBrowserDetector();

    public:
        static TBrowserDetector& Instance();
        const TString& GetBrowser(const TString& userAgent, TString* browserVersion = nullptr);
        const TString& GetOS(const TString& userAgent, TString* osVersion = nullptr);
        std::map<std::string, std::string> GetUatraitsData(const TString& userAgent);

        bool InitializeUatraits(const char* uaconfigPath);
        bool InitializeUatraitsFromMemory(const TStringBuf uaconfigData);
    };

}
