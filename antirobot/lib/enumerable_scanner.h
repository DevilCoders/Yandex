#pragma once

#include <library/cpp/regex/pire/pire.h>

#include <library/cpp/iterator/functools.h>

#include <util/generic/xrange.h>


namespace NAntiRobot {


class TEnumerableScanner : public NPire::TNonrelocScanner {
public:
    using NPire::TNonrelocScanner::TNonrelocScanner;

    TEnumerableScanner(NPire::TNonrelocScanner base) // NOLINT(google-explicit-constructor)
        : NPire::TNonrelocScanner(std::move(base))
    {}

    auto EnumerateStates() const {
        return NFuncTools::Map([this] (size_t i) {
            return static_cast<State>(IndexToState(i));
        }, xrange(Size()));
    }

    bool Matches(TStringBuf s) const {
        return Final(NPire::Runner(*this).Run(s).State());
    }
};


} // namespace NAntiRobot
