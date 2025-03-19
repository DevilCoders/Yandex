#pragma once

#include        <util/system/defaults.h>
#include        <util/generic/vector.h>

#include        "id2string.h"

class TIndexUrlExtractor : public IId2String
{
private:
    typedef TVector<char*> TUrls;
    TUrls Urls;

public:
    TIndexUrlExtractor(const TString& index);
    TString GetString(ui32 docId) override;
    bool GetString(ui32 docId, TString* url);
    size_t GetSize() const;
    ~TIndexUrlExtractor() override;
};
