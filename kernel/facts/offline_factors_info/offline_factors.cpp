#include "offline_factors.h"

#include <library/cpp/resource/resource.h>
#include <search/session/searcherprops.h>
#include <util/generic/hash_set.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>


namespace NFacts {

static const THashSet<TString> OFFLINE_FACT_SNIPPET_SOURCES = {
    // sources based on offline fact snippet
    "fact_snippet_offline",
    "offline_fs_fact",
    "url_ofs"
};

TOfflineFactorsInfo::TOfflineFactorsInfo() {
    const TString jsonContent = NResource::Find("offline_factors_info");
    SourceToFactorNames = NSc::TValue::FromJsonThrow(jsonContent);
};

NSc::TValue TOfflineFactorsInfo::Parse(TStringBuf factSource, const NSc::TArray& factorValues) const {
    const TString factSourceAsValidKey = TSearcherProps::ConvertToValidKey(factSource, '_');
    NSc::TValue factorNames;
    if (OFFLINE_FACT_SNIPPET_SOURCES.contains(factSourceAsValidKey)) {
        factorNames = SourceToFactorNames.Get("ofs");
    } else {
        Y_ENSURE(SourceToFactorNames.Has(factSourceAsValidKey), "Invalid source key for offline factors");
        factorNames = SourceToFactorNames.Get(factSourceAsValidKey);
    }

    NSc::TValue parsedFactors;
    size_t factorCount = std::min(factorValues.size(), factorNames.GetArray().size());
    for (size_t i = 0; i < factorCount; ++i) {
        parsedFactors[TString(factorNames[i].GetString())] = factorValues[i].GetNumber();
    }
    return parsedFactors;
}

}  // namespace NFacts
