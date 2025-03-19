#pragma once

#include "attribute_fetcher.h"
#include "fetchers.h"

#include <util/generic/ptr.h>

namespace NHtmlXPath {

class TUniversalAttributeFetcher : public IAttributeValueFetcher {
public:
    TUniversalAttributeFetcher(ELanguage language = LANG_UNK);
    bool CanFetch(EAttributeType type) const override;
    void Fetch(const NXml::TxmlXPathObjectPtr& attributeData, const NJson::TJsonValue& attributeMetadata, TAttributes& attributes) override;

    void Reset(ELanguage language);
private:
    TVector< TSimpleSharedPtr<IAttributeValueFetcher> > Fetchers;
    TSimpleSharedPtr<IAttributeValueFetcher> DateFetcher;
};

} // namespace NHtmlXPath

