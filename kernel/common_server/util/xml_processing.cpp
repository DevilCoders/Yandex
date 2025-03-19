#include "xml_processing.h"

void TXmlProcessor::FindAllNodesByName(const NXml::TConstNode node, const TString& nodeName, TVector<NXml::TConstNode>& result) {
    if (node.IsNull()) {
        return;
    }
    if (node.Name() == nodeName) {
        result.push_back(node);
    }
    auto child = node.FirstChild();
    while (!child.IsNull()) {
        FindAllNodesByName(child, nodeName, result);
        child = child.NextSibling();
    }
}

TVector<NXml::TConstNode> TXmlProcessor::FindNodesByName(const NXml::TConstNode& node, const TString& nodeName) {
    TVector<NXml::TConstNode> result;
    FindAllNodesByName(node, nodeName, result);
    return result;
}

NXml::TConstNode TXmlProcessor::FindFirstNodeByName(const NXml::TConstNode& node, const TString& nodeName) {
    auto result = FindNodesByName(node, nodeName);
    if (result.size() == 0) {
        return NXml::TConstNode();
    }
    return result.front();
}