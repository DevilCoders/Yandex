#pragma once

#include <library/cpp/xml/document/xml-document.h>

class TXmlProcessor {
private:
    static void FindAllNodesByName(const NXml::TConstNode node, const TString& nodeName, TVector<NXml::TConstNode>& result);

public:
    static TVector<NXml::TConstNode> FindNodesByName(const NXml::TConstNode& node, const TString& nodeName);
    static NXml::TConstNode FindFirstNodeByName(const NXml::TConstNode& node, const TString& nodeName);
};
