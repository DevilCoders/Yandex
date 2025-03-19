#pragma once

#include "attribute_fetcher.h"

#include <kernel/xpathmarker/xmlwalk/xmlwalk.h>
#include <kernel/xpathmarker/xmlwalk/sanitize.h>
#include <kernel/xpathmarker/utils/debug.h>

#include <util/string/cast.h>

namespace NHtmlXPath {

class TBooleanAttributeFetcher : public IAttributeValueFetcher {
public:
    bool CanFetch(EAttributeType type) const override {
        return type == AT_BOOLEAN;
    }

    void Fetch(const NXml::TxmlXPathObjectPtr& attributeData, const NJson::TJsonValue& attributeMetadata, TAttributes& attributes) override {
        if (attributeData->type != XPATH_BOOLEAN) {
            XPATHMARKER_ERROR("attributeData given to TBooleanAttributeFetcher doesn't hava XPATH_BOOLEAN type");
            ythrow yexception() << "attributeData given to TBooleanAttributeFetcher doesn't hava XPATH_BOOLEAN type";
        }

        if (!attributeData->boolval) {
            XPATHMARKER_INFO("TBooleanAttributeFetcher got false result");
            if (attributeMetadata[ALLOW_FALSE_ATTRIBUTE].GetBooleanRobust()) {
                XPATHMARKER_INFO("It's okay accoring to config (" << ALLOW_FALSE_ATTRIBUTE << " is set true)")
            } else {
                return;
            }
        }

        attributes.push_back(TAttribute(attributeMetadata[NAME_ATTRIBUTE].GetStringRobust(), attributeData->boolval ? "1" : "0", AT_BOOLEAN));
    }

private:
    static const char* ALLOW_FALSE_ATTRIBUTE;
};

} // namespace NHtmlXPath

