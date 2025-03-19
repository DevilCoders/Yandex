#pragma once

#include <library/cpp/scheme/scheme.h>

namespace NFactsSerpParser {

    // Extract human-readable fact answer text from serpData.
    // Empty string if no text can be extracted.
    TString ExtractAnswer(const NSc::TValue& serpData);

} // namespace NFactsSerpParser
