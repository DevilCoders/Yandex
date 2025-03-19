#include "xpathmarker.h"

#include <kernel/xpathmarker/fetchers/universalfetcher.h>
#include <kernel/xpathmarker/utils/debug.h>

#include <library/cpp/html/norm/norm.h>

#include <library/cpp/html/storage/storage.h>
#include <library/cpp/html/tree/buildxml_xpath.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/utility.h>
#include <util/stream/file.h>

#include <cstring>


using namespace NJson;
using namespace NXml;
using namespace NHtml::NXmlWalk;
using namespace NHtmlTree;

namespace NHtmlXPath {

namespace {
    static const size_t MIN_HOST_LENGTH = 3;

    // @returns true on success
    bool TryGetHostFromRegexp(const TString& regexp, TString& host) {
        static const char HTTP_REGEXP_PREFIX[] = "http://(www\\.)?";
        static const char HTTPS_REGEXP_PREFIX[] = "https://(www\\.)?";
        static const char HTTP_HTTPS_REGEXP_PREFIX[] = "http(s)?://(www\\.)?";

        TStringBuf base(regexp);
        if (regexp.StartsWith(HTTP_REGEXP_PREFIX)) {
            base.Skip(Y_ARRAY_SIZE(HTTP_REGEXP_PREFIX) - 1);
        } else if (regexp.StartsWith(HTTPS_REGEXP_PREFIX)) {
            base.Skip(Y_ARRAY_SIZE(HTTPS_REGEXP_PREFIX) - 1);
        } else if (regexp.StartsWith(HTTP_HTTPS_REGEXP_PREFIX)) {
            base.Skip(Y_ARRAY_SIZE(HTTP_HTTPS_REGEXP_PREFIX) - 1);
        } else {
            XPATHMARKER_INFO(regexp << " has neither https:// nor http:// prefix")

            return false;
        }

        // The host ends at '/' or the end of the string; unescape '.'
        host.clear();
        for (const char* ch = base.data(); *ch != '/' && *ch != '\0'; ++ch) {
            if (*ch == '\\' && *(ch + 1) == '.') {
                ++ch;
            }
            host.push_back(*ch);
        }

        XPATHMARKER_INFO("Got " << host << " from regexp " << regexp << " is_length_acceptable = " << (host.length() > MIN_HOST_LENGTH))
        return host.length() >= MIN_HOST_LENGTH;
    }

    bool TryGetHostFromUrl(const TString& url, TString& host) {
        host = CutWWWPrefix(GetOnlyHost(url));

        return host.length() >= MIN_HOST_LENGTH;
    }

};

void TXPathMarker::CheckVersion(const TJsonValue& config) const {
    static const char* VERSION_KEY = "version";
    static const ui32  MIN_SUPPORTED_VERSION = 2;
    static const ui32  MAX_SUPPORTED_VERSION = 6;

    const TJsonValue& versionValue = config[VERSION_KEY];
    ui32 version = versionValue.GetInteger();

    XPATHMARKER_INFO("Config has version " << version)

    if (version < MIN_SUPPORTED_VERSION) {
        XPATHMARKER_ERROR("Config has version " << version << " but MIN_SUPPORTED_VERSION is" << MIN_SUPPORTED_VERSION)

        ythrow yexception() << "XPath config file version too old";
    }
    if (version > MAX_SUPPORTED_VERSION) {
        XPATHMARKER_ERROR("Config has version " << version << " but MAX_SUPPORTED_VERSION is" << MIN_SUPPORTED_VERSION)

        ythrow yexception() << "XPath config file version too new";
    }
}

void TXPathMarker::ParseUrlPattern(const TJsonValue& confItem, size_t regexpNo) {
    static const char* URL_REGEXP_KEY = "url_pattern";

    const TString& urlRegexp = confItem[URL_REGEXP_KEY].GetString();
    XPATHMARKER_INFO("Read URL regexp " << urlRegexp)

    // Build a scanner for this URL
    TUrlScanner::TScanner newUrlScanner;
    try {
        newUrlScanner = TUrlScanner::CreateScanner(urlRegexp);
    } catch (const yexception& e) {
        XPATHMARKER_ERROR("Can't construct scanner from regexp " << urlRegexp)

        ythrow yexception() << "errorneous pire regexp " << urlRegexp <<
            " (" << e.what() << ") encountered while processing Xpath markup config";
    }

    // Check whether it can be attributed to one host
    TString host;
    if (TryGetHostFromRegexp(urlRegexp, host)) {
        XPATHMARKER_INFO("URL regexp " << urlRegexp << " is attributed to host " << host)

        ScannerForHost[host].AddScanner(newUrlScanner, regexpNo);
    } else {
        XPATHMARKER_INFO("URL regexp " << urlRegexp << " is not host-specific")

        UniversalScanner.AddScanner(newUrlScanner, regexpNo);
    }
}

void TXPathMarker::ParseZoneMapping(const TJsonValue& confItem) {
    static const char* ZONES_SECTION = "zones";

    TZoneConfigs* configs = new TZoneConfigs();

    const TJsonValue::TArray& zones = confItem[ZONES_SECTION].GetArray();

    XPATHMARKER_INFO("Got " << zones.size() << " zone record(s) in zones sections")

    for (size_t zoneNo = 0; zoneNo < zones.size(); ++zoneNo) {
        const TJsonValue& zoneConf = zones[zoneNo];
        configs->PushBack(new TZoneConfig(zoneConf));
    }

    XpathConfig.PushBack(configs);
}

void TXPathMarker::ParseConfig(const TJsonValue& config) {
    static const char* DATA_SECTION = "data";

    const TJsonValue::TArray& data = config[DATA_SECTION].GetArray();

    for (size_t regexpNo = 0; regexpNo < data.size(); ++regexpNo) {
        XPATHMARKER_INFO("Reading config entry " << regexpNo)

        const TJsonValue& confItem = data[regexpNo];

        ParseUrlPattern(confItem, regexpNo);
        ParseZoneMapping(confItem);
    }

    XPATHMARKER_INFO("Host-agnostic regexps found: " << UniversalScanner.GetRegexpsNumber())
}

// The entries in @XpathConf and the regexps in @UrlScanner are numbered accordingly
void TXPathMarker::Configure(const TString& file) {
    TJsonValue config;
    {
        XPATHMARKER_INFO("Trying to read JSON config from " << file)

        TIFStream configStream(file);
        if (!ReadJsonTree(&configStream, &config))
            ythrow yexception() << "can't read XPath config JSON file " << file << " (malformed JSON?)";
    }

    XPATHMARKER_INFO("Checking version of config")
    CheckVersion(config);

    XpathConfig.Clear();
    UniversalScanner = TUrlScanner();

    XPATHMARKER_INFO("Parsing the config")
    ParseConfig(config);

    XPATHMARKER_INFO("Configuring is done")
}

/// Checks if given URL matches any of the given regexps.
/// Returns NULL iff it doesn't match anything,
/// otherwise a pointer to TZoneMapping of the regexp
/// that matches the given url (the last one in case of a tie)
const TXPathMarker::TZoneConfigs* TXPathMarker::GetMatchingEntry(const TString& url) const {
    // First, try to match a host-specific regexp
    TString host;
    TScanners::const_iterator scannerForHost;
    if (TryGetHostFromUrl(url, host) &&
        (scannerForHost = ScannerForHost.find(host)) != ScannerForHost.end())
    {
        XPATHMARKER_INFO("URL " << url << " will be checked for host regexps of " << host)

        const TUrlScanner& scanner = scannerForHost->second;
        const TUrlScanner::TScanResult scanResult = scanner.Scan(url);

        if (scanner.HasMatches(scanResult)) {
            const size_t theMatch = scanner.GetMaxMatch(scanResult);

            XPATHMARKER_INFO("URL " << url << " has matched " << host << " regexp #" << theMatch)

            return XpathConfig[theMatch];
        }
    }

    // If that failed, try a generic regexp
    {
        const TUrlScanner::TScanResult scanResult = UniversalScanner.Scan(url);
        if (! UniversalScanner.HasMatches(scanResult)) {
            // No matches
            return nullptr;
        }

        const size_t theMatch = UniversalScanner.GetMaxMatch(scanResult);
        XPATHMARKER_INFO("URL " << url << " has matched regexp #" << theMatch)

        return XpathConfig[theMatch];
    }
}

void TXPathMarker::MarkZones(const TDocument& document, INumeratorInvoker& numeratorInvoker, IZoneWriter& zoneWriter) const {
    XPATHMARKER_INFO("Starting marking zones for " << document.Url << " (language = " << NameByLanguage(document.Language) << ")")

    const TZoneConfigs* configs = GetMatchingEntry(document.Url);

    if (configs != nullptr) {
        XPATHMARKER_INFO("Got config for url " << document.Url)

        TSimpleSharedPtr<TXmlTree> tree = BuildDOM(numeratorInvoker, document.DocProps);

        AttributeFetcher.Reset(document.Language);

        DoMarkZones(*tree, *configs, zoneWriter);

        XPATHMARKER_INFO("Zone marking for " << document.Url << " is finished")
    } else {
        XPATHMARKER_INFO("Have no config for url " << document.Url)
    }
}

TSimpleSharedPtr<TXmlTree> TXPathMarker::BuildDOM(INumeratorInvoker& numeratorInvoker, const IParsedDocProperties& docProps) const {
   TXmlDomBuilder treeBuilder(docProps);
   numeratorInvoker.Numerate(treeBuilder);
   return treeBuilder.GetTree();
}

void TXPathMarker::DoMarkZones(TXmlTree& tree, const TZoneConfigs& zones, IZoneWriter& zoneWriter) const {
    TXmlDoc& xml = tree.GetXmlDoc();

    for (TZoneConfigs::const_iterator zone = zones.begin(); zone != zones.end(); ++zone) {
        TZoneConfig::TZones zones = (*zone)->Fetch(xml, AttributeFetcher);
        for (size_t zoneNo = 0; zoneNo < zones.size(); ++zoneNo) {
            zoneWriter.WriteZone(*zones[zoneNo]);
        }
    }
    zoneWriter.FinishWriting();
}

} // namespace NHtmlXPath

