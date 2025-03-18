#include "regex_detector.h"

#include <library/cpp/iterator/enumerate.h>


namespace NAntiRobot {


TRegexDetector::TRegexDetector(
    TConstArrayRef<TString> patterns,
    size_t maxScannerSize
) {
    BaseToScanner.reserve(patterns.size());

    for (const auto& [i, pattern] : Enumerate(patterns)) {
        BaseToScanner.emplace_back(i, NPire::TLexer(pattern).Parse().Compile<NPire::TNonrelocScanner>());
    }

    GlueScanners(
        &BaseToScanner,
        [] (auto& scanner) -> NPire::TNonrelocScanner& {
            return scanner.second;
        },
        {},
        maxScannerSize
    );
}

TVector<size_t> TRegexDetector::Detect(TStringBuf s) const {
    TVector<size_t> ret;

    for (const auto& [base, scanner] : BaseToScanner) {
        const auto state = NPire::Runner(scanner).Run(s).State();
        if (scanner.Final(state)) {
            auto [begin, end] = scanner.AcceptedRegexps(state);
            for (; begin != end; ++begin) {
                ret.push_back(base + *begin);
            }
        }
    }

    return ret;
}


} // namespace NAntiRobot
