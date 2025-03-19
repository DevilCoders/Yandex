#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/system/tls.h>

#include <algorithm>

class TStringPool {
public:
    bool Has(TStringBuf value) const;
    TString Get(TStringBuf value);
    TString GetDebugString() const;
private:
    TVector<TString> Strings;
};
