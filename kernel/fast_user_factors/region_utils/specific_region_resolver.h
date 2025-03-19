#pragma once

#include <kernel/geo/utils.h>
#include <kernel/geo/types/region_types.h>

#include <util/folder/path.h>

namespace NFastUserFactors {
    enum class ERelevRegionType {
        Common,
        Large
    };

    template<NFastUserFactors::ERelevRegionType RelevRegionType>
    class TSpecificRelevRegionResolver {
    private:
        static TString GetRelevRegionFilename() {
            switch (RelevRegionType) {
                case ERelevRegionType::Common:
                    return "relev_regions.txt";
                case ERelevRegionType::Large:
                    return "relev_regions_large.txt";
            }
        }

        static TGeoRegion GetDefaultRegion() {
            return 0; // World
        }
    public:
        TSpecificRelevRegionResolver(const TString& workingDirectory, const TString& geodataFilename):
            RelevRegionResolver(geodataFilename, JoinFsPaths(workingDirectory, this->GetRelevRegionFilename()), GetDefaultRegion())
        {
        }

        const TRelevRegionResolver* operator->() const {
            return &RelevRegionResolver;
        }
    private:
        const TRelevRegionResolver RelevRegionResolver;
    };

    const TRelevRegionResolver* GetRelevRegionsResolver(const ERelevRegionType relevRegionType, const TString& workingDirectory, const TString& geodataFilename);
}
