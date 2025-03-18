#pragma once

#include "regex_glue.h"

#include <library/cpp/regex/pire/pire.h>

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NAntiRobot {


class TRegexDetector {
public:
    TRegexDetector() = default;

    explicit TRegexDetector(
        TConstArrayRef<TString> patterns,
        size_t maxScannerSize = DEFAULT_MAX_SCANNER_SIZE
    );

    TVector<size_t> Detect(TStringBuf s) const;

    size_t NumScanners() const {
        return BaseToScanner.size();
    }

private:
    TVector<std::pair<size_t, NPire::TNonrelocScanner>> BaseToScanner;
};


} // namespace NAntiRobot
