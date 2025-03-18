#pragma once

#include <cstddef>
#include <util/generic/string.h>

namespace NReverseGeocoder {
    namespace NBorderExtractor {
        struct TInitParams {
            TString InputPath;
            TString WantedIds;
            TString SavePath;
            bool CheckOnly;

            TInitParams() = default;
            TInitParams(int argc, const char* argv[]);
        };

        void Extract(const TInitParams& params);
    }
}
