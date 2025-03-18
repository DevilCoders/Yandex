#pragma once

#include <util/generic/string.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        struct TConfig {
            TString InputPath;
            TString OutputPath;
            TString ExcludePath;
            TString SubstPath;
            bool SaveRawBorders = false;
            bool SaveKvs = true;

            TConfig() = default;
            TConfig(int argc, const char* argv[]);
        };
    }
}
