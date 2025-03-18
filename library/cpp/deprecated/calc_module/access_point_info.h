#pragma once

#include <util/generic/set.h>
#include <util/generic/string.h>

class TAccessPointInfo {
private:
    TSet<TString> UsedPoints;

public:
    void AddUsedPoint(const TString& name);
    const TSet<TString>& GetUsedPoints() const noexcept {
        return UsedPoints;
    }
};
