#pragma once


#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/hash_set.h>

namespace NIndexAnn {

    const static size_t DEFAULT_MAX_TEXTS = (1 << 15) - 2; // BREAK_LEVEL_Max - 1

    class IFeaturesProfile {
    public:
        virtual bool IsEssential(size_t type) const = 0;

        virtual ~IFeaturesProfile() {
        }
    };

    class TExcludedProfile : public IFeaturesProfile {
        THolder<const IFeaturesProfile> Profile;
        THashSet<size_t> ExcludedStreams;
    public:
        TExcludedProfile(const IFeaturesProfile* profile, const THashSet<size_t>& streams);
        bool IsEssential(size_t type) const override;
    };

    class TComboProfile : public IFeaturesProfile {
        TVector<THolder<const IFeaturesProfile>> Profiles;

    public:
        TComboProfile(std::initializer_list<const IFeaturesProfile*> profiles = {});

        void AddProfile(const IFeaturesProfile* profile);
        bool IsEssential(size_t type) const override;
    };

    class TAnyFeaturesProfile : public IFeaturesProfile {
    public:
        bool IsEssential(size_t type) const override;
    };

} // NIndexAnn

