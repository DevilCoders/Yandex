#pragma once

#include <util/generic/hash.h>
#include <util/generic/maybe.h>

#include <library/cpp/json/json_value.h>

class IFactorsInfo {
public:
    virtual ~IFactorsInfo() = default;

    virtual size_t GetFactorCount() const = 0;
    virtual bool GetFactorIndex(const char* name, size_t* index) const = 0;
    TMaybe<size_t> GetFactorIndex(const char* name) const {
        size_t index;
        return GetFactorIndex(name, &index) ? MakeMaybe(index) : Nothing();
    }
    virtual const char* GetFactorName(size_t index) const = 0;
    virtual const char* const* GetFactorNames() const = 0;
    virtual const float* GetFactorCanonicalValues() const {
        return nullptr;
    }
    virtual const char* GetFactorInternalName(size_t /*index*/) const {
        return "";
    }
    virtual const char* GetFactorSliceName(size_t /*index*/) const {
        return "";
    }
    virtual TMaybe<size_t> GetL3ModelValueIndex() const {
        return TMaybe<size_t>();
    }

    virtual bool IsOftenZero(size_t /*index*/) const {
        return false;
    }
    virtual bool IsBinary(size_t /*index*/) const {
        return false;
    }
    virtual bool IsStaticFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsTextFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsLinkFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsQueryFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsBrowserFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsUserFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsUnusedFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsUnimplementedFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsDeprecatedFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsRemovedFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsNot01Factor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsMetaFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsTransFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsFastrankFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsLocalizedFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsStaticReginfoFactor(size_t /*index*/) const {
        return false;
    }
    virtual bool IsVideoClickAdditionFactor(size_t /*index*/) const {
        return false;
    }
    virtual ui32 GetFactorType(size_t /*index*/) const {
        return 0;
    }
    virtual float GetCanonicalValue(size_t /*index*/) const {
        return 0.f;
    }
    virtual float GetMinValue(size_t /*index*/) const {
        return 0.f;
    }
    virtual float GetMaxValue(size_t /*index*/) const {
        return 1.f;
    }
    virtual bool HasTagName(size_t /*index*/, const TStringBuf& /*name*/) const {
        return false;
    }

    virtual bool HasGroupName(size_t /*index*/, const TStringBuf& /*name*/) const {
        return false;
    }

    virtual bool HasTagId(size_t /*index*/, int /*tag*/) const {
        return false;
    }
    virtual void GetTagNames(size_t /*index*/, TVector<TString>& /*tagNames*/) const {
    }
    virtual NJson::TJsonValue GetExtJson(size_t /*index*/) const {
        return NJson::TJsonValue(NJson::JSON_NULL);
    }
    virtual void GetFactorSources(size_t /*index*/, THashMap<TString, TString>& /*sources*/) const {
    }
    virtual const char* GetFactorExpression(size_t /*index*/) const {
        return "";
    }
    virtual void GetDependencyNames(size_t /*index*/, THashMap<TString, TVector<TString>>& /*dependencies*/) const {
    }
};

template <class TFactorInfo>
class TSimpleFactorsInfo: public IFactorsInfo {
public:
    TSimpleFactorsInfo(size_t factorCount, const TFactorInfo* factorInfo)
        : FactorCount(factorCount)
        , FactorInfo(factorInfo)
    {
        Names.resize(factorCount);
        CanonicalValues.resize(factorCount);
        for (size_t i = 0; i < FactorCount; ++i) {
            FactorNameToIndex[FactorInfo[i].Name] = i;
            Names[i] = FactorInfo[i].Name;
            CanonicalValues[i] = FactorInfo[i].CanonicalValue;
        }
    }

    size_t GetFactorCount() const override {
        return FactorCount;
    }

    const char* GetFactorName(size_t index) const override {
        if (index >= FactorCount)
            return nullptr;
        return FactorInfo[index].Name;
    }

    const char* GetFactorInternalName(size_t index) const override {
        if (index >= FactorCount)
            return nullptr;
        return FactorInfo[index].InternalName;
    }

    bool GetFactorIndex(const char* name, size_t* index) const override {
        const size_t* indexPtr = FactorNameToIndex.FindPtr(name);
        if (!indexPtr)
            return false;
        *index = *indexPtr;
        return true;
    }

    const char* const* GetFactorNames() const override {
        return Names.data();
    }

    const float* GetFactorCanonicalValues() const override {
        return CanonicalValues.data();
    }

    //template here lets FactorInfo to miss TagsData
    template<typename TTag>
    bool HasTag(size_t index, const TTag& tag) const {
        // This fixes problems in SEARCH-1013
        if (index >= FactorCount)
            return false;
        return FactorInfo[index].TagsData.TagsIds.contains(tag);
    }
protected:
    size_t FactorCount = 0;
    const TFactorInfo* FactorInfo = nullptr;

    THashMap<const char*, size_t> FactorNameToIndex;
    TVector<const char*> Names;
    TVector<float> CanonicalValues;
};
