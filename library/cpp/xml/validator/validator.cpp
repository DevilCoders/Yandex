#include "validator.h"

using namespace NXml;

void TValidator::AddXmlCatalog(const TString& catalogFile) {
    const auto errorCode = xmlLoadCatalog(catalogFile.c_str());
    Y_ENSURE(!errorCode, "Failed to load catalog file " << catalogFile);
}

TValidator::TValidator(const TString& schemaUrl) {
    SchemaParserCtxtPtr.Reset(xmlSchemaNewParserCtxt(schemaUrl.c_str()));
    Y_ENSURE(SchemaParserCtxtPtr, "Failed to create Schema parser context");

    SchemaPtr.Reset(xmlSchemaParse(SchemaParserCtxtPtr.Get()));
    Y_ENSURE(SchemaPtr, "Failed to parse schema " << schemaUrl);
}

bool TValidator::IsValid(TDocument& doc) const {
    TxmlSchemaValidCtxtPtr validCtxtPtr(xmlSchemaNewValidCtxt(SchemaPtr.Get()));
    Y_ENSURE(validCtxtPtr, "Failed to create validation context");

    const auto errorCode = xmlSchemaValidateDoc(validCtxtPtr.Get(), doc.GetImpl());

    Y_ENSURE(errorCode >= 0, "Internal error occurred while validation XML");
    return !errorCode;
}
