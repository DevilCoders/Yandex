#pragma once

#include <util/generic/fwd.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/types.h>

namespace NMetaProtocol {
    class TReport;
};

namespace NFacts {

    struct TFConsoleFactRule {
        TString Type;
        TVector<TString> Fields;
        ui32 TimestampSeconds;
    };

    void SerializeFConsoleRulesToReport(const TVector<TFConsoleFactRule>& rules, NMetaProtocol::TReport& dst);

    TVector<TVector<TString>> FetchFConsoleFactRules(const THashMap<TString, TString>& searcherProps);

};
