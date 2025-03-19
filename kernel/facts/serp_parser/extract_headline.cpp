#include "extract_headline.h"

namespace NFactsSerpParser {

    TString ExtractHeadline(const NSc::TValue& serpData) {
        return TString{serpData["headline"].GetString()};
    }

}
