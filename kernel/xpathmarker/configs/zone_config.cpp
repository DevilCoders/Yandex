#include "zone_config.h"

#include <libxml/tree.h>

#include <kernel/xpathmarker/xmlwalk/xmlwalk.h>

#include <library/cpp/html/tree/xmltree.h>


using namespace NHtml::NXmlWalk;
using NHtmlTree::TNodeData;
using NXml::TxmlXPathObjectPtr;

namespace NHtmlXPath {

const char* TZoneConfig::ZONE_NAME_KEY = "name";
const char* TZoneConfig::EXPORT_TYPES_KEY = "export";
const char* TZoneConfig::BASE_XPATH_KEY = "base_xpath";
const char* TZoneConfig::XPATH_KEY = "xpath";
const char* TZoneConfig::ZONE_ATTRS_KEY = "attrs";

TZoneConfig::TZoneConfig(const TJsonValue& jsonConfig)
    : Xpath(jsonConfig[XPATH_KEY].GetString())
    , BaseXpath(jsonConfig[BASE_XPATH_KEY].GetString())
    , Name(jsonConfig[ZONE_NAME_KEY].GetString())
    , ExportType(ParseExportTypes(jsonConfig[EXPORT_TYPES_KEY].GetString()))
{
    const TJsonValue::TArray& attrs = jsonConfig[ZONE_ATTRS_KEY].GetArray();

    for (size_t attrNo = 0; attrNo < attrs.size(); ++attrNo) {
        AttributeConfigs.push_back(TAttributeConfig(attrs[attrNo]));
    }
}

TZoneConfig::TZones TZoneConfig::Fetch(TXmlDoc& xmlDoc, IAttributeValueFetcher& fetcher) const {
    TZones resultZones;

    bool zoneXpathIsAbsolute = BaseXpath.empty();

    const TString& expr = zoneXpathIsAbsolute ? Xpath : BaseXpath;
    TxmlXPathObjectPtr result = xmlDoc.Select(expr);
    xmlNodeSetPtr nodes = result->nodesetval; // got set of selected nodes
    int nodesSelected = xmlXPathNodeSetGetLength(nodes);

    for (int nodeNo = 0; nodeNo < nodesSelected; ++nodeNo) {
        xmlNodePtr baseNode = xmlXPathNodeSetItem(nodes, nodeNo);

        // zoneNode - node from which we will extract zone
        // if there is no relative xpath then we alread found it - it is baseNode
        xmlNodePtr zoneNode = baseNode;

        if (!zoneXpathIsAbsolute) { // have relative xpath
            TxmlXPathObjectPtr relativeXpathResult = xmlDoc.Select(Xpath, baseNode);
            if (relativeXpathResult->type != XPATH_NODESET) {
                // We selected not nodes, so skipping
                continue;
            }

            xmlNodeSetPtr relativeXpathNodeSet = relativeXpathResult->nodesetval;
            size_t relativeXpathNodeSetLength = xmlXPathNodeSetGetLength(relativeXpathNodeSet);
            if (relativeXpathNodeSetLength != 1) {
                // We selected more than one node (or none at all), can't handle this
                continue;
            }

            zoneNode = xmlXPathNodeSetItem(relativeXpathNodeSet, 0); // will extract zone from the only node
        }

        if (IsTextNode(zoneNode)) {
            // Can't start zone at text node. Moving to parent
            if (IsSingleChild(zoneNode)) {
                zoneNode = zoneNode->parent;
            } else {
                // This node has siblings. Can't decide where to start zone (it's possible to catch more than should be catched if we just move to parent)
                continue;
            }
        }

        // _private field contains zone-specific data, can't proceed without it
        if (zoneNode->_private == nullptr) {
            continue;
        }

        // Ready to extract zone

        const TNodeData* nodeData = (const TNodeData*) zoneNode->_private;
        TZone* zone = new TZone(Name, ExportType, nodeData->StartPosition, nodeData->EndPosition);

        // Extracting attributes
        for (size_t attrNo = 0; attrNo < AttributeConfigs.size(); ++attrNo) {
            AttributeConfigs[attrNo].Fetch(xmlDoc, zoneNode, *zone, fetcher);
        }

        resultZones.push_back(TSimpleSharedPtr<TZone>(zone));
    }

    return resultZones;
}

} // namespace NHtmlXPath


