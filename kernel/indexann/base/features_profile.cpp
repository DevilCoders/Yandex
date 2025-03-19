#include "features_profile.h"

using namespace NIndexAnn;

TExcludedProfile::TExcludedProfile(const IFeaturesProfile* profile, const THashSet<size_t>& streams)
    : Profile(profile)
    , ExcludedStreams(streams)
{
}


bool TExcludedProfile::IsEssential(size_t type) const {
    return ExcludedStreams.contains(type) ? false : Profile->IsEssential(type);
}

TComboProfile::TComboProfile(std::initializer_list<const IFeaturesProfile*> profiles) {
    for (const auto& ptr : profiles) {
        AddProfile(ptr);
    }
}

void TComboProfile::AddProfile(const IFeaturesProfile* profile) {
    Profiles.emplace_back(profile);
}

bool TComboProfile::IsEssential(size_t type) const {
    for (const auto& profile : Profiles) {
        if (profile->IsEssential(type)) {
            return true;
        }
    }
    return false;
}

bool TAnyFeaturesProfile::IsEssential(size_t /*type*/) const {
    return true;
}

