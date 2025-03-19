#pragma once

#include "urlscanner.h"
#include "configs/zone_config.h"
#include "entities/all.h"
#include "fetchers/universalfetcher.h"
#include "utils/debug.h"
#include "zonewriters/zonewriter.h"

#include <libxml/tree.h>

#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/tree/xmltree.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/regex/libregex/regexstr.h>
#include <library/cpp/xml/doc/xmldoc.h>

#include <util/draft/holder_vector.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>


namespace NHtmlXPath {

class INumeratorInvoker {
public:
    virtual void Numerate(INumeratorHandler& numeratorHandler) = 0;

    ~INumeratorInvoker() {
    }
};

class TXPathMarker : public TNonCopyable {
public:
    TXPathMarker(const TString& filename = TString()) {
        XPATHMARKER_DEBUG("TXPathMarker is compiled with debug output on, which will dramatically decrease efficiency. This mode must not be used in production")
        if (! filename.empty()) {
            Configure(filename);
        }
    }

    void Configure(const TString& filename);

    void MarkZones(
        const TDocument& document,
        INumeratorInvoker& numeratorInvoker,
        IZoneWriter& zoneWriter
    ) const;

private:
    typedef THolderVector<const TZoneConfig> TZoneConfigs;
    typedef THolderVector<const TZoneConfigs> TXpathConfig;

    typedef THashMap<TString, TUrlScanner> TScanners;

private:
    void CheckVersion(const NJson::TJsonValue& config) const;

    void ParseConfig(const NJson::TJsonValue& config);
    void ParseUrlPattern(const TJsonValue& confItem, size_t regexpNo);
    void ParseZoneMapping(const NJson::TJsonValue& confItem);

    const TZoneConfigs* GetMatchingEntry(const TString& url) const;

    void DoMarkZones(
        NHtmlTree::TXmlTree& tree,
        const TZoneConfigs& zones,
        IZoneWriter& zoneWriter
        ) const;

    TSimpleSharedPtr<NHtmlTree::TXmlTree> BuildDOM(
        INumeratorInvoker& numeratorInvoker,
        const IParsedDocProperties& docProps) const;

private:
    TXpathConfig XpathConfig;
    mutable TUniversalAttributeFetcher AttributeFetcher; // allows to reset language in dater

    TUrlScanner UniversalScanner;
    TScanners ScannerForHost;
};

} // namespace NHtmlXPath

