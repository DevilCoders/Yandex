#pragma once

#include <library/cpp/scheme/scheme.h>

namespace NFactsSerpParser {

    // Extract fact headline text from serpData.
    // Empty string if no text can be extracted.
    TString ExtractHeadline(const NSc::TValue& serpData);

} // namespace NFactsSerpParser
