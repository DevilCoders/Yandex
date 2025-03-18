#pragma once

#include <kernel/generated_factors_info/metadata/factors_metadata.pb.h>
#include <search/web/blender/factors_info/metadata/factors_metadata.pb.h>

#include <kernel/web_factors_info/factor_names.h>
#include <search/web/blender/factors_info/factor_names.h>

#include <util/generic/bitmap.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>

class IFactorsSet {
private:
    TString Description;
    size_t FactorsCount;
    TVector<TString> FactorsNames;
    TVector<THashSet<int>> FactorsTags;

public:
    IFactorsSet(const TString& description)
        : Description(description)
    {
    }

    virtual ~IFactorsSet() {}

    template <class T>
    void FillFactorsSet(const T* info, const size_t count) {
        FactorsCount = count;
        FactorsNames.resize(FactorsCount);
        FactorsTags.resize(FactorsCount);

        for (size_t idx = 0; idx < FactorsCount; ++idx) {
            FactorsNames[idx] = info[idx].Name;
            FactorsTags[idx] = info[idx].TagsData.TagsIds;
        }
    }

    TString GetFactorName(size_t factor) const {
        return FactorsNames[factor];
    }

    size_t GetFactorCount() const {
        return FactorsCount;
    }

    TString GetDescription() const {
        return Description;
    }

    const TVector<TString>& GetFactorsNames() const {
        return FactorsNames;
    }

    const TVector<THashSet<int>>& GetFactorsTags() const {
        return FactorsTags;
    }

    virtual bool IsDeprecatedFactor(size_t factor) const = 0;
    virtual bool IsRemovedFactor(size_t factor) const = 0;
    virtual bool IsUnusedFactor(size_t factor) const = 0;
    virtual bool IsRearrangeFactor(size_t factor) const = 0;
    virtual bool IsUnimplementedFactor(size_t factor) const = 0;
};

class TWebFactorsSet: public IFactorsSet {
public:
    TWebFactorsSet(const TString& description)
        : IFactorsSet(description)
    {
        FillFactorsSet<TFactorInfo>(GetFactorsInfo(), GetWebFactorsInfo()->GetFactorCount());
    }

    bool IsDeprecatedFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(TFactorInfo::TG_DEPRECATED)
                || GetFactorsTags()[factor].contains(TFactorInfo::TG_REMOVED);
    }

    bool IsRemovedFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(TFactorInfo::TG_REMOVED)
                || (GetFactorsTags()[factor].contains(TFactorInfo::TG_DEPRECATED)
                    && GetFactorsTags()[factor].contains(TFactorInfo::TG_UNIMPLEMENTED));
    }

    bool IsUnusedFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(TFactorInfo::TG_UNUSED)
                || GetFactorsTags()[factor].contains(TFactorInfo::TG_DEPRECATED)
                || GetFactorsTags()[factor].contains(TFactorInfo::TG_REMOVED);
    }

    bool IsRearrangeFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(TFactorInfo::TG_REARR_USE);
    }

    bool IsUnimplementedFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(TFactorInfo::TG_UNIMPLEMENTED)
                || GetFactorsTags()[factor].contains(TFactorInfo::TG_REMOVED);
    }
};

class TBlenderFactorsSet: public IFactorsSet {
public:
    TBlenderFactorsSet(const TString& description)
        : IFactorsSet(description)
    {
        FillFactorsSet<NBlender::TFactorInfo>(NBlender::GetFactorsInfo(), NBlender::GetBlenderFactorsInfo()->GetFactorCount());
    }

    bool IsDeprecatedFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(NBlender::TFactorInfo::TG_DEPRECATED);
    }

    bool IsRemovedFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(NBlender::TFactorInfo::TG_REMOVED);
    }

    bool IsUnusedFactor(size_t /*factor*/) const override {
        return false;
    }

    bool IsRearrangeFactor(size_t /*factor*/) const override {
        return false;
    }

    bool IsUnimplementedFactor(size_t factor) const override {
        return GetFactorsTags()[factor].contains(NBlender::TFactorInfo::TG_UNIMPLEMENTED);
    }
};
