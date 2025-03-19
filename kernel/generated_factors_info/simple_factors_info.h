#pragma once

#include <kernel/factors_info/factors_info.h>

#include <library/cpp/json/json_reader.h>

#include <util/stream/mem.h>

template <class TFactorInfo>
class TSimpleSearchFactorsInfo: public TSimpleFactorsInfo<TFactorInfo> {
    using TBase = TSimpleFactorsInfo<TFactorInfo>;
    using TBase::FactorCount;
    using TBase::FactorInfo;

public:
    TSimpleSearchFactorsInfo(size_t factorCount, const TFactorInfo* factorInfo)
        : TSimpleFactorsInfo<TFactorInfo>(factorCount, factorInfo)
    {
        for (size_t i = 0; i < factorCount; ++i) {
            if (TSimpleFactorsInfo<TFactorInfo>::HasTag(i, TFactorInfo::TG_L3_MODEL_VALUE)) {
                L3ModelValueIndex = i;
                break;
            }
        }
    }

    TMaybe<size_t> GetL3ModelValueIndex() const override {
        return L3ModelValueIndex;
    }

    bool IsOftenZero(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_OFTEN_ZERO);
    }

    bool IsBinary(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_BINARY);
    }

    bool IsStaticFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_STATIC);
    }

    bool IsTextFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_DOC_TEXT);
    }

    bool IsLinkFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_LINK_GRAPH) ||
            this->HasTag(index, TFactorInfo::TG_LINK_TEXT);
    }

    bool IsQueryFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_QUERY_ONLY);
    }

    bool IsBrowserFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_BROWSER);
    }

    bool IsUserFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_USER);
    }

    bool IsUnusedFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_UNUSED);
    }

    bool IsUnimplementedFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_UNIMPLEMENTED);
    }

    bool IsDeprecatedFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_DEPRECATED);
    }

    bool IsRemovedFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_REMOVED);
    }

    bool IsNot01Factor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_NOT_01);
    }

    bool IsMetaFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_TRANS);
    }

    bool IsTransFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_TRANS);
    }

    bool IsFastrankFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_QUERY_ONLY) ||
            this->HasTag(index, TFactorInfo::TG_STATIC) ||
            this->HasTag(index, TFactorInfo::TG_L2);
    }

    bool IsLocalizedFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_LOCALIZED_COUNTRY) ||
               this->HasTag(index, TFactorInfo::TG_LOCALIZED_REGION) ||
               this->HasTag(index, TFactorInfo::TG_LOCALIZED_CITY) ||
               this->HasTag(index, TFactorInfo::TG_LOCALIZED_GEOINQUERY);
    }

    bool IsStaticReginfoFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_STATIC_REGINFO);
    }

    bool IsVideoClickAdditionFactor(size_t index) const override {
        return this->HasTag(index, TFactorInfo::TG_VIDEO_CLICK_ADDITION_USE);
    }

    const char* GetFactorSliceName(size_t index) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return "";
        const auto& info = FactorInfo[index];
        return info.SliceName;
    }

    float GetCanonicalValue(size_t index) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return 0.f;
        return FactorInfo[index].CanonicalValue;
    }

    float GetMinValue(size_t index) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return 0.f;
        return FactorInfo[index].MinValue;
    }

    float GetMaxValue(size_t index) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return 1.f;
        return FactorInfo[index].MaxValue;
    }

    bool HasTagName(size_t index, const TStringBuf& name) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return false;
        return FactorInfo[index].TagsData.TagsNames.contains(name);
    }

    bool HasGroupName(size_t index, const TStringBuf& name) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return false;
        return FactorInfo[index].GroupsData.GroupNames.contains(name);
    }

    bool HasTagId(size_t index, int tag) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return false;
        return FactorInfo[index].TagsData.TagsIds.contains(tag);
    }

    virtual void GetFactorSources(size_t index, THashMap<TString, TString>& sourcesData) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return;
        for (auto&& [arg, name] : FactorInfo[index].MoveFromFactorSource.Sources.SourceArgToClassName) {
            sourcesData[arg] = name;
        }
    }

    virtual const char* GetFactorExpression(size_t index) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return "";
        return FactorInfo[index].MoveFromFactorSource.Expression;
    }

    void GetDependencyNames(size_t index, THashMap<TString, TVector<TString>>& dependencies) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return;
        dependencies = FactorInfo[index].DependsOn.DependentSlices;
    }

    void GetTagNames(size_t index, TVector<TString>& tagNames) const override {
        if (Y_UNLIKELY(index >= FactorCount))
            return;
        for (const auto& tagName : FactorInfo[index].TagsData.TagsNames) {
            tagNames.push_back(tagName);
        }
    }

    NJson::TJsonValue GetExtJson(size_t index) const override {
        if (Y_LIKELY(index < FactorCount)) {
            const auto& info = FactorInfo[index];
            TMemoryInput in(TStringBuf(info.ExtJson));
            NJson::TJsonValue result;
            if (Y_LIKELY(NJson::ReadJsonTree(&in, &result))) {
                return result;
            }
        }
        return NJson::TJsonValue(NJson::JSON_NULL);
    }

private:
    TMaybe<size_t> L3ModelValueIndex;
};

template <class TFactorInfo>
class TWebFactorsInfo : public TSimpleSearchFactorsInfo<TFactorInfo> {
public:
    TWebFactorsInfo(size_t factorCount, const TFactorInfo* factorInfo)
        : TSimpleSearchFactorsInfo<TFactorInfo>(factorCount, factorInfo)
    {}

    TWebFactorsInfo(size_t begin, size_t end, const TFactorInfo* factorInfo)
        : TSimpleSearchFactorsInfo<TFactorInfo>(end - begin, factorInfo + begin)
    {
    }
};
