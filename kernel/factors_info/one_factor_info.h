#pragma once

#include "factors_info.h"

class TOneFactorInfo {
private:
    size_t Index = 0;
    const IFactorsInfo* Info = nullptr;

public:
    TOneFactorInfo() = default;

    TOneFactorInfo(size_t index, const IFactorsInfo* info)
        : Index(index)
        , Info(info)
    {}

    operator bool () const {
        return !!Info;
    }

    size_t GetIndex() const {
        return Index;
    }
    const IFactorsInfo& GetInfo() const {
        Y_ASSERT(Info);
        return *Info;
    }

    bool IsExist() const {
        Y_ASSERT(Info);
        if (Info->GetFactorCount() <= Index)
            return false;
        return true;
    }

    const char* GetFactorName() const {
        Y_ASSERT(Info);
        return Info->GetFactorName(Index);
    }
    const char* GetFactorInternalName() const {
        Y_ASSERT(Info);
        return Info->GetFactorInternalName(Index);
    }
    const char* GetFactorSliceName() const {
        Y_ASSERT(Info);
        return Info->GetFactorSliceName(Index);
    }

    bool IsOftenZero() const {
        Y_ASSERT(Info);
        return Info->IsOftenZero(Index);
    }
    bool IsBinary() const {
        Y_ASSERT(Info);
        return Info->IsBinary(Index);
    }
    bool IsStaticFactor() const {
        Y_ASSERT(Info);
        return Info->IsStaticFactor(Index);
    }
    bool IsTextFactor() const {
        Y_ASSERT(Info);
        return Info->IsTextFactor(Index);
    }
    bool IsLinkFactor() const {
        Y_ASSERT(Info);
        return Info->IsLinkFactor(Index);
    }
    bool IsQueryFactor() const {
        Y_ASSERT(Info);
        return Info->IsQueryFactor(Index);
    }
    bool IsBrowserFactor() const {
        Y_ASSERT(Info);
        return Info->IsBrowserFactor(Index);
    }
    bool IsUserFactor() const {
        Y_ASSERT(Info);
        return Info->IsUserFactor(Index);
    }
    bool IsUnusedFactor() const {
        Y_ASSERT(Info);
        return Info->IsUnusedFactor(Index);
    }
    bool IsUnimplementedFactor() const {
        Y_ASSERT(Info);
        return Info->IsUnimplementedFactor(Index);
    }
    bool IsDeprecatedFactor() const {
        Y_ASSERT(Info);
        return Info->IsDeprecatedFactor(Index);
    }
    bool IsRemovedFactor() const {
        Y_ASSERT(Info);
        return Info->IsRemovedFactor(Index);
    }
    bool IsNot01Factor() const {
        Y_ASSERT(Info);
        return Info->IsNot01Factor(Index);
    }
    bool IsMetaFactor() const {
        Y_ASSERT(Info);
        return Info->IsMetaFactor(Index);
    }
    bool IsTransFactor() const {
        Y_ASSERT(Info);
        return Info->IsTransFactor(Index);
    }
    bool IsFastrankFactor() const {
        Y_ASSERT(Info);
        return Info->IsFastrankFactor(Index);
    }
    ui32 GetFactorType() const {
        Y_ASSERT(Info);
        return Info->GetFactorType(Index);
    }
    float GetCanonicalValue() const {
        Y_ASSERT(Info);
        return Info->GetCanonicalValue(Index);
    }
    float GetMinValue() const {
        Y_ASSERT(Info);
        return Info->GetMinValue(Index);
    }
    float GetMaxValue() const {
        Y_ASSERT(Info);
        return Info->GetMaxValue(Index);
    }
    bool HasTagName(const TStringBuf& name) const {
        Y_ASSERT(Info);
        return Info->HasTagName(Index, name);
    }
    bool HasGroupName(const TStringBuf& name) const {
        Y_ASSERT(Info);
        return Info->HasGroupName(Index, name);
    }
    bool HasTagId(int tag) const {
        Y_ASSERT(Info);
        return Info->HasTagId(Index, tag);
    }
    void GetTagNames(TVector<TString>& tagNames) const {
        Y_ASSERT(Info);
        Info->GetTagNames(Index, tagNames);
    }
    NJson::TJsonValue GetExtJson() const {
        Y_ASSERT(Info);
        return Info->GetExtJson(Index);
    }
};
