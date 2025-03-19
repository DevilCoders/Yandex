#pragma once

#include "scheme.h"

#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/util/json_processing.h>
#include <util/string/split.h>

class TAttributedEntityDefaultFieldNames {
public:
    static const TString GetAttributesFieldName() {
        return "groupping_attributes";
    }
    static const TString GetObjectFieldName() {
        return "attributed_entity";
    }
};

template <class TFieldNames>
class TAttributedEntity {
    RTLINE_ACCEPTOR_DEF(TAttributedEntity, GrouppingTags, TSet<TString>);
public:
    Y_WARN_UNUSED_RESULT bool DeserializeAttributes(const NJson::TJsonValue& jsonSettings) {
        return TJsonProcessor::ReadContainer(jsonSettings, TFieldNames::GetAttributesFieldName(), GrouppingTags);
    }

    static void FillAttributesScheme(const IBaseServer& server, const TString& settingsKey, NFrontend::TScheme& result) {
        const TString attributesStr = server.GetSettings().GetValueDef<TString>("administration.attributes." + settingsKey, "");
        const TVector<TString> variants = StringSplitter(attributesStr).SplitBySet(", ").SkipEmpty().ToList<TString>();
        result.Add<TFSVariants>(TFieldNames::GetAttributesFieldName(), "Аттрибуты для группировки").SetVariants(variants).SetEditable(variants.empty()).SetMultiSelect(true).SetRequired(false);
    }

    void SerializeAttributes(NJson::TJsonValue& result) const {
        TJsonProcessor::WriteContainerArray(result, TFieldNames::GetAttributesFieldName(), GrouppingTags);
    }

    void InsertObjectToJson(NJson::TJsonValue& result) const {
        result.InsertValue(TFieldNames::GetObjectFieldName(), SerializeObjectToJson());
    }

    NJson::TJsonValue SerializeObjectToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        TJsonProcessor::WriteContainerArray(result, TFieldNames::GetAttributesFieldName(), GrouppingTags);
        return result;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeObjectFromJson(const NJson::TJsonValue& jsonInfo) {
        if (jsonInfo.Has(TFieldNames::GetObjectFieldName())) {
            return DeserializeAttributes(jsonInfo[TFieldNames::GetObjectFieldName()]);
        } else {
            return true;
        }
    }

    static NFrontend::TScheme GetScheme(const IBaseServer& server, const TString& settingsKey) {
        NFrontend::TScheme result;
        FillAttributesScheme(server, settingsKey, result);
        return result;
    }

    static void InsertScheme(const IBaseServer& server, const TString& settingsKey, NFrontend::TScheme& scheme) {
        scheme.Add<TFSStructure>(TFieldNames::GetObjectFieldName(), "атрибуты объекта").SetStructure(GetScheme(server, settingsKey));
    }
};
