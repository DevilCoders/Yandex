#pragma once

#include "attribute_fetcher.h"

#include <kernel/xpathmarker/xmlwalk/xmlwalk.h>
#include <kernel/xpathmarker/xmlwalk/sanitize.h>
#include <kernel/xpathmarker/utils/debug.h>

#include <util/string/cast.h>

namespace NHtmlXPath {

class TIntegerAttributeFetcher : public IAttributeValueFetcher {
public:
    bool CanFetch(EAttributeType type) const override {
        return type == AT_INTEGER;
    }

    void Fetch(const NXml::TxmlXPathObjectPtr& attributeData, const NJson::TJsonValue& attributeMetadata, TAttributes& attributes) override {
        TString text = SerializeToStroka(attributeData);
        CompressWhiteSpace(text);
        DecodeHTMLEntities(text);

        TString result;
        for (size_t i = 0; i < text.size(); ++i) {
            if (IsDigit(text[i])) {
                result += text[i];
            }
        }

        if (!!result) {
            XPATHMARKER_INFO("TIntegerAttributeFetcher got '" << result << "' from text = '" << text << "'")

            attributes.push_back(TAttribute(attributeMetadata[NAME_ATTRIBUTE].GetStringRobust(), result, AT_INTEGER));
        } else {
            XPATHMARKER_INFO("TIntegerAttributeFetcher got no result from text = '" << text << "'")
        }
    }

private:

    static bool IsDigit(char c) {
        return c >= '0' && c <= '9';
    }
};

} // namespace NHtmlXPath

