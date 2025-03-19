#pragma once

#include "public.h"

#include <cloud/blockstore/config/features.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TFeaturesConfig
{
    struct TFeatureInfo
    {
        THashSet<TString> CloudIds;
        THashSet<TString> FolderIds;
        bool IsBlacklist;
    };

private:
    const NProto::TFeaturesConfig Config;

    THashMap<TString, TFeatureInfo> Features;

public:
    TFeaturesConfig(NProto::TFeaturesConfig config = {});

    bool IsValid() const;

    bool IsFeatureEnabled(
        const TString& cloudId,
        const TString& folderId,
        const TString& featureName) const;
};

}   // namespace NCloud::NBlockStore::NStorage
