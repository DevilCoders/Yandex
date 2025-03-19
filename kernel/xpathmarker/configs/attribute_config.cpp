#include "attribute_config.h"

#include <kernel/xpathmarker/utils/debug.h>

#include <libxml/tree.h>

using NXml::TxmlXPathObjectPtr;

namespace NHtmlXPath {

const char* TAttributeConfig::ZONE_ATTR_NAME_KEY = "name";
const char* TAttributeConfig::ZONE_ATTR_TYPE_KEY = "type";
const char* TAttributeConfig::ZONE_ATTR_XPATH_KEY = "xpath";

TAttributeConfig::TAttributeConfig(const TJsonValue& jsonConfig)
    : AttributeMetadata(jsonConfig)
    , Xpath(jsonConfig[ZONE_ATTR_XPATH_KEY].GetString())
    , Name(jsonConfig[ZONE_ATTR_NAME_KEY].GetString())
    , Type(GetType(jsonConfig[ZONE_ATTR_TYPE_KEY].GetString()))
{}


void TAttributeConfig::Fetch(TXmlDoc& xmlDoc, xmlNodePtr baseNode, TZone& zone, IAttributeValueFetcher& fetcher) const {
    TxmlXPathObjectPtr attributeData = xmlDoc.Select(Xpath, baseNode);

    size_t wereAttributes = zone.Attributes.size();

    XPATHMARKER_INFO("Fetching attribute " << Name << " of type " << Type)
    fetcher.Fetch(attributeData, AttributeMetadata, zone.Attributes);

    for (size_t attrNo = wereAttributes; attrNo < zone.Attributes.size(); ++attrNo) {
        XPATHMARKER_INFO("Setting start position for attribute " << zone.Attributes[attrNo].Name << " equal to start position of zone (" << zone.StartPosition << ")")
        zone.Attributes[attrNo].Position = zone.StartPosition;
    }
}

} // namespace NHtmlXPath

