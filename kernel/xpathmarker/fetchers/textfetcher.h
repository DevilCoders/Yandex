#pragma once

#include "attribute_fetcher.h"

#include <kernel/xpathmarker/xmlwalk/utils.h>
#include <kernel/xpathmarker/xmlwalk/sanitize.h>
#include <kernel/xpathmarker/utils/debug.h>

namespace NHtmlXPath {

class TTextAttributeFetcher : public IAttributeValueFetcher {
public:
    bool CanFetch(EAttributeType type) const override {
        return type == AT_TEXT;
    }

    void Fetch(const NXml::TxmlXPathObjectPtr& attributeData, const NJson::TJsonValue& attributeMetadata, TAttributes& attributes) override {
        TString separator = " ";
        attributeMetadata[SEPARATOR_ATTRIBUTE].GetString(&separator);
        TString regexp = "";
        attributeMetadata[REGEXP_ATTRIBUTE].GetString(&regexp);

        TString text = SerializeToStroka(attributeData, separator, regexp);
        CompressWhiteSpace(text);
        DecodeHTMLEntities(text);

        if (text.empty()) {
            XPATHMARKER_INFO("TTextAttributeFetcher got no text")
            if (attributeMetadata[ALLOW_EMPTY_ATTRIBUTE].GetBooleanRobust()) {
                XPATHMARKER_INFO("It's okay accoring to config (" << ALLOW_EMPTY_ATTRIBUTE << " is set true)")
            } else {
                return;
            }
        }

        attributes.push_back(TAttribute(attributeMetadata[NAME_ATTRIBUTE].GetStringRobust(), text, AT_TEXT));

        XPATHMARKER_INFO("TTextAttributeFetcher got '" << text << "'")
    }

    ~TTextAttributeFetcher() override {
    }
private:
    static const char* SEPARATOR_ATTRIBUTE;
    static const char* REGEXP_ATTRIBUTE;
    static const char* ALLOW_EMPTY_ATTRIBUTE;
};


} // namespace NHtmlXPath

