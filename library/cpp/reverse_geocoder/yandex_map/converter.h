#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NReverseGeocoder {
    namespace NYandexMap {
        struct TConfig {
            bool Gz;
            int Jobs;
            TStringBuf SkipKindList;
            int CountryId;
            TStringBuf GeoDataFileName;
            TStringBuf GeoMappingFileName;
        };

        void Convert(const TString& inputPath, const TString& outputPath, const TConfig& config);
    }
}
