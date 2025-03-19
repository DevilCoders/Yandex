#include "extract_url.h"
#include <library/cpp/scheme/scheme.h>

namespace NFactsSerpParser {

    static TString GetObjectFactUrl(const NSc::TValue& serpData)
    {
        return TString{serpData["base_info"]["url"].GetString()};
    }

    TString ExtractUrl(const NSc::TValue& serpData)
    {
        const TStringBuf& type = serpData["type"].GetString();

        if (type == "entity-fact") {
            return GetObjectFactUrl(serpData);
        }

        return TString{serpData["url"].GetString()};
    }

} // namespace NFactsSerpParser
