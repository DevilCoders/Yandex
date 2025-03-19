#pragma once

#include <kernel/xpathmarker/entities/attribute.h>
#include <kernel/xpathmarker/entities/attribute_traits.h>
#include <kernel/xpathmarker/entities/zone.h>
#include <kernel/xpathmarker/fetchers/attribute_fetcher.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/xml/doc/xmldoc.h>

using NJson::TJsonValue;
using NXml::TXmlDoc;

namespace NHtmlXPath {

class TAttributeConfig {
public:
    explicit TAttributeConfig(const TJsonValue& jsonConfig);

    void Fetch(TXmlDoc& xmlDoc, xmlNodePtr baseNode, TZone& zone, IAttributeValueFetcher& fetcher) const;

private:
    TJsonValue AttributeMetadata;
    TString Xpath;
    TString Name;
    EAttributeType Type;

private:
    static const char* ZONE_ATTR_NAME_KEY;
    static const char* ZONE_ATTR_TYPE_KEY;
    static const char* ZONE_ATTR_XPATH_KEY;
};

typedef TVector<TAttributeConfig> TAttributeConfigs;

} // namespace NHtmlXPath

