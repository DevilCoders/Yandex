#include "xmltree.h"

using NXml::TXmlDoc;

namespace NHtmlTree {
    TXmlTree::TXmlTree()
        : PrivateFieldStorage(STORAGE_INITIAL_SIZE)
    {
    }

    TXmlDoc& TXmlTree::GetXmlDoc() {
        return XmlDoc;
    }

    TNodeData* TXmlTree::AllocateNodeData() {
        return new (PrivateFieldStorage) TNodeData();
    }

}
