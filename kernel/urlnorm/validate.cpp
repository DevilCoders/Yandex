#include "validate.h"

#include <library/cpp/uri/uri.h>
#include <library/cpp/uri/http_url.h>

#include <util/string/printf.h>

bool IsValidRobotUrl(const TStringBuf& url, TString& errorDescription) {
    using namespace NUri;

    TUri uri;
    const TState::EParsed state = uri.Parse(url, TFeature::FeaturesRecommended);
    if (state != TState::ParsedOK) {
        errorDescription = Sprintf("Parsing failed: state=%d.",
                                   static_cast<int>(state));
        return false;
    }

    if (uri.IsNull(TField::FlagHost | TField::FlagScheme)) {
        errorDescription = "Scheme is missing or bad host.";
        return false;
    }

    return true;
}
