#include "universalfetcher.h"

#include <kernel/xpathmarker/xmlwalk/utils.h>
#include <kernel/xpathmarker/utils/debug.h>

#include <util/generic/yexception.h>

namespace NHtmlXPath {

TUniversalAttributeFetcher::TUniversalAttributeFetcher(ELanguage language /*= LANG_UNK*/) {
    Fetchers.push_back(new TTextAttributeFetcher);
    Fetchers.push_back(new TBooleanAttributeFetcher);
    Fetchers.push_back(new TIntegerAttributeFetcher);
    Fetchers.push_back(DateFetcher = new TDateAttributeFetcher(language));

    XPATHMARKER_INFO("TUniversalAttributeFetcher has TTextAttributeFetcher, TIntegerAttributeFetcher, TBooleanAttributeFetcher, TDateAttributeFetcher(language = " << NameByLanguage(language) << ")")
}

bool TUniversalAttributeFetcher::CanFetch(EAttributeType type) const {
    for (size_t i = 0; i < Fetchers.size(); ++i) {
        if (Fetchers[i]->CanFetch(type)) {
            return true;
        }
    }
    XPATHMARKER_INFO("TUniversalAttributeFetcher can't fetch attribute with type " << type)
    return false;
}

void TUniversalAttributeFetcher::Fetch(const NXml::TxmlXPathObjectPtr& attributeData, const NJson::TJsonValue& attributeMetadata, TAttributes& attributes) {
    EAttributeType type = GetType(attributeMetadata[TYPE_ATTRIBUTE].GetStringRobust());
    for (size_t i = 0; i < Fetchers.size(); ++i) {
        if (Fetchers[i]->CanFetch(type)) {
            Fetchers[i]->Fetch(attributeData, attributeMetadata, attributes);
            return;
        }
    }
    XPATHMARKER_ERROR("TUniversalAttibuteFetcher can't fetch attribute with type " << type)
    ythrow yexception() << "Can't process attribute (unknown type '" << int(type) << "')";
}

void TUniversalAttributeFetcher::Reset(ELanguage language) {
    ((TDateAttributeFetcher*) DateFetcher.Get())->Reset(language);
}

} // namespace NHtmlXPath

