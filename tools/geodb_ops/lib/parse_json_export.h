#pragma once

#include <util/generic/fwd.h>

namespace NLastGetopt {
    class TOpts;
}

class IInputStream;
class IOutputStream;

namespace NGeoDBBuilder {
    struct TBuilderConfig {
        bool WithoutNames = {};
        bool WithoutLocation = {};
        bool WithoutSpan = {};
        bool WithoutPhoneCode = {};
        bool WithoutTimeZone = {};
        bool WithoutShortName = {};
        bool WithoutPopulation = {};
        bool WithoutAmbiguous = {};
        bool WithoutServices = {};
        bool WithoutNativeName = {};
        bool WithoutSynonymNames = {};

        NLastGetopt::TOpts GetLastGetopt();
    };

    void ParseGeobaseJSONExport(const TBuilderConfig& options,
                                IInputStream& input, IOutputStream& output);

    TString BuildURLForJSONExport(const TString& hostName);
}  // namespace NGeoDBBuilder
