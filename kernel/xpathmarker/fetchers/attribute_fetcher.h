#pragma once

#include <kernel/xpathmarker/entities/attribute.h>

#include <libxml/tree.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/xml/doc/xmltypes.h>

#include <util/generic/string.h>

namespace NHtmlXPath {

class IAttributeValueFetcher {
public:
    // attributeData - result of xpath applying
    // attributeMetadata - json-value from config for given attribute
    // attributes - attribute storage where new attribute(s) should be added
    virtual void Fetch(const NXml::TxmlXPathObjectPtr& attributeData, const NJson::TJsonValue& attributeMetadata, TAttributes& attributes) = 0;
    virtual bool CanFetch(EAttributeType type) const = 0;
    virtual ~IAttributeValueFetcher() {
    }

protected:
    // name for type attribute in metadata JSON
    static const char* TYPE_ATTRIBUTE;
    static const char* NAME_ATTRIBUTE;
};

}

