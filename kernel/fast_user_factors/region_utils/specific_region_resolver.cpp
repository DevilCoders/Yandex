#include "specific_region_resolver.h"

namespace NFastUserFactors {
    const TRelevRegionResolver* GetRelevRegionsResolver(const ERelevRegionType relevRegionType, const TString& workingDirectory, const TString& geodataFilename) {
        switch (relevRegionType) {
            case ERelevRegionType::Common:
                return Singleton<TSpecificRelevRegionResolver<ERelevRegionType::Common>>(workingDirectory, geodataFilename)->operator->();
            case ERelevRegionType::Large:
                return Singleton<TSpecificRelevRegionResolver<ERelevRegionType::Large>>(workingDirectory, geodataFilename)->operator->();
        }
    }
}
