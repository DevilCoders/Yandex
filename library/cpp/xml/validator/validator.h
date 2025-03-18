#pragma once

#include <library/cpp/xml/init/ptr.h>
#include <library/cpp/xml/document/xml-document.h>

#include <contrib/libs/libxml/include/libxml/catalog.h>

namespace NXml {
    class TValidator {
    public:
        /**
        * This method is no thread safe, catalog initialization
        * should preferably be done once at startup
        */
        static void AddXmlCatalog(const TString& catalogFile);

        explicit TValidator(const TString& schemaUrl);

        bool IsValid(TDocument& doc) const;

    private:
        TxmlSchemaParserCtxtPtr SchemaParserCtxtPtr;
        TxmlSchemaPtr SchemaPtr;
    };
}
