#include "features_config.h"

namespace NCloud::NBlockStore::NStorage {

TFeaturesConfig::TFeaturesConfig(
        NProto::TFeaturesConfig config)
    : Config(std::move(config))
{
    for (const auto& feature: Config.GetFeatures()) {
        TFeatureInfo info;
        info.IsBlacklist = !feature.HasWhitelist();

        const auto& cloudList =
            (feature.HasBlacklist() ? feature.GetBlacklist() : feature.GetWhitelist());

        for (const auto& cloudId: cloudList.GetCloudIds()) {
            info.CloudIds.emplace(cloudId);
        }

        for (const auto& folderId: cloudList.GetFolderIds()) {
            info.FolderIds.emplace(folderId);
        }

        Features.emplace(feature.GetName(), std::move(info));
    }
}

bool TFeaturesConfig::IsValid() const
{
    return true;
}

bool TFeaturesConfig::IsFeatureEnabled(
    const TString& cloudId,
    const TString& folderId,
    const TString& featureName) const
{
    auto it = Features.find(featureName);
    if (it != Features.end()) {
        auto isBlacklist = it->second.IsBlacklist;
        if (it->second.CloudIds.contains(cloudId)
                || it->second.FolderIds.contains(folderId))
        {
            return !isBlacklist;
        }
        return isBlacklist;
    }

    return false;
}

}   // namespace NCloud::NBlockStore::NStorage
