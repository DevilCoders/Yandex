#pragma once

#include "attribute_config.h"

#include <kernel/xpathmarker/entities/zone.h>
#include <kernel/xpathmarker/entities/zone_traits.h>
#include <kernel/xpathmarker/fetchers/attribute_fetcher.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/xml/doc/xmldoc.h>

#include <util/generic/ptr.h>

using NJson::TJsonValue;
using NXml::TXmlDoc;

namespace NHtmlXPath {

class TZoneConfig {
public:
    typedef TVector< TSimpleSharedPtr<TZone> > TZones;

public:
    explicit TZoneConfig(const TJsonValue& jsonConfig);

    TZones Fetch(TXmlDoc& xmlDoc, IAttributeValueFetcher& fetcher) const;

private:
    TString Xpath;
    TString BaseXpath;
    TString Name;
    EExportType ExportType;
    TAttributeConfigs AttributeConfigs;

private:
    static const char* ZONE_NAME_KEY;
    static const char* EXPORT_TYPES_KEY;
    static const char* BASE_XPATH_KEY;
    static const char* XPATH_KEY;
    static const char* ZONE_ATTRS_KEY;
};

} // namespace NHtmlXPath

