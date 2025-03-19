#pragma once

#include <library/cpp/scheme/scheme.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>


namespace NFacts {

class TOfflineFactorsInfo {
public:
    TOfflineFactorsInfo();
    NSc::TValue Parse(TStringBuf factSource, const NSc::TArray& factorValues) const;
private:
    NSc::TValue SourceToFactorNames;
};

}  // namespace NFacts
