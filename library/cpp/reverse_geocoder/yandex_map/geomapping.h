#pragma once

#include <map>
#include <unordered_map>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NReverseGeocoder {
    namespace NYandexMap {
        struct TMappingTraits {
            TString ToString() const;

            int RegionId{};
            int RegType{};
            TString RegNamePath;

            long ToponymId{};
            TString Kind;
            TString ToponymPath;

            long SourceId{-1};
        };

        using TTopId2RegIdMapping = std::unordered_map<long, int>;
        using Id2ToponymMappingType = std::map<int, TMappingTraits>;

        Id2ToponymMappingType LoadGeoMapping(TStringBuf mappingFileMame, TTopId2RegIdMapping& sid2regidMap, TTopId2RegIdMapping& topid2regidMap);
    }
}
