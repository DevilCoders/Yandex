#include "json.h"

#include <library/cpp/json/json_reader.h>

#include <util/stream/mem.h>
#include <util/string/vector.h>

void NUtil::TJsonFilter::AddFilters(const TCgiParameters& cgi, const TString& prefix /*= TString()*/) {
    for (size_t i = 0; i < cgi.NumOfValues("filter"); ++i) {
        TVector<TString> filterPortion = SplitString(cgi.Get("filter", i), ",");
        if (prefix) {
            for (const TString& f : filterPortion) {
                if (f.StartsWith(prefix)) {
                    Filters.insert(f.substr(prefix.length()));
                }
            }
        } else
            Filters.insert(filterPortion.begin(), filterPortion.end());
    }
}

const NJson::TJsonValue& NUtil::TJsonFilter::ConvertSource(TStringBuf source, NJson::TJsonValue& tempSource) {
    TMemoryInput input(source.data(), source.size());
    NJson::ReadJsonTree(&input, &tempSource, true);
    return tempSource;
}

const NJson::TJsonValue& NUtil::TJsonFilter::ConvertSource(const NJson::TJsonValue& source, NJson::TJsonValue& /*tempSource*/) {
    return source;
}
