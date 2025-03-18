#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NSolveAmbig {
    enum ERankCheck {
        RC_G_COVERAGE /* "greater-coverage" */,
        RC_G_WEIGHT /* "greater-weight" */,
        RC_L_COUNT /* "less-count" */,
    };

    class TRankMethod: public TVector<ERankCheck> {
    public:
        explicit TRankMethod() = default;
        explicit TRankMethod(const TRankMethod& rankMethod) = default;
        explicit TRankMethod(size_t num, ...);
        explicit TRankMethod(const TStringBuf& s);
        TRankMethod& operator=(const TRankMethod& rankMethod) = default;
    };

    const TRankMethod& DefaultRankMethod();

}

Y_DECLARE_OUT_SPEC(inline, NSolveAmbig::TRankMethod, output, rankMethod) {
    bool first = true;
    for (NSolveAmbig::TRankMethod::const_iterator iRankCheck = rankMethod.begin(); iRankCheck < rankMethod.end(); ++iRankCheck) {
        if (!first) {
            output << ',';
        } else {
            first = false;
        }
        output << *iRankCheck;
    }
}
