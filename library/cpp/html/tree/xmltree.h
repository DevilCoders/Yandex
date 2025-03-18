#pragma once

#include <library/cpp/numerator/numerate.h>
#include <library/cpp/xml/doc/xmldoc.h>

#include <util/memory/pool.h>

namespace NHtmlTree {
    // other data may appear later
    struct TNodeData: public TPoolable {
        TPosting StartPosition;
        TPosting EndPosition;

        TNodeData()
            : StartPosition(0)
            , EndPosition(0)
        {
        }
    };

    class TXmlTree: public TNonCopyable {
    public:
        TXmlTree();

        NXml::TXmlDoc& GetXmlDoc();

        TNodeData* AllocateNodeData();

    private:
        NXml::TXmlDoc XmlDoc;
        TMemoryPool PrivateFieldStorage;

    private:
        static const size_t STORAGE_INITIAL_SIZE = 4096;
    };

}
