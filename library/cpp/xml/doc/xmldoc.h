#pragma once

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/array_ref.h>
#include <util/generic/yexception.h>

#include "xmltypes.h"

namespace NXml {
    class TXmlNodeSet;
    class TXmlNode;

    /// @todo: use single NXml::TError everywhere
    class TXmlError: public yexception {
    };
    class TXPathError: public TXmlError {
    };

    /// represents libxml2 parsed document
    class TXmlDoc {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        TXmlDoc();                             ///< construct empty document
        TXmlDoc(const TArrayRef<const char>& doc);       ///< construct document from memory block text
        TXmlDoc(const char* text, size_t len); ///< construct document from memory block text
        ~TXmlDoc();

        void Save(IOutputStream& to);

        TxmlXPathObjectPtr Select(const TString& expr); ///< run select with XPath expression
        TxmlXPathObjectPtr Select(const TString& expr, const xmlNodePtr context);
        xmlNode* GetRoot();
        void SetRoot(xmlNode* node);
    };

}
