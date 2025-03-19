#include <kernel/common_server/library/startrek/entity.h>

#include <library/cpp/json/json_reader.h>
#include <kernel/common_server/util/json_processing.h>

namespace {
    static NJson::TJsonValue FilterNonEmptyStrings(const NJson::TJsonValue& arrayValue) {
        NJson::TJsonArray filteredItems;

        for (const auto& innerValue : arrayValue.GetArray()) {
            if (innerValue.IsString() && !!innerValue.GetString()) {
                filteredItems.AppendValue(innerValue);
            }
        }

        return filteredItems;
    }
}

const NJson::TJsonValue* TStartrekTicket::GetAdditionalValue(const TString& path) const {
    const NJson::TJsonValue* valuePtr = nullptr;

    TStringBuf pathBuf(path);
    auto head = pathBuf.NextTok('.');

    if (AdditionalAttributes.contains(head)) {
        const NJson::TJsonValue& tree = AdditionalAttributes.at(head);

        if (!path.empty()) {
            valuePtr = tree.GetValueByPath(pathBuf);
        } else {
            valuePtr = &tree;
        }
    }

    return valuePtr;
}

bool TStartrekTicket::GetAdditionalValue(const TString& path, TString& value) const {
    const NJson::TJsonValue* valuePtr = GetAdditionalValue(path);

    if (valuePtr && valuePtr->IsString()) {
        value = valuePtr->GetString();
        return true;
    }

    return false;
}

bool TStartrekTicket::GetAdditionalValue(const ETicketField& field, TString& value) const {
    return GetAdditionalValue(ToString(field), value);
}

TString TStartrekTicket::GetAdditionalValueDef(const TString& path, const TString& defaultValue) const {
    TString value;
    if (!GetAdditionalValue(path, value)) {
        value = defaultValue;
    }
    return value;
}

TString TStartrekTicket::GetAdditionalValueDef(const ETicketField& field, const TString& defaultValue) const {
    return GetAdditionalValueDef(ToString(field), defaultValue);
}

bool TStartrekTicket::SetAdditionalValue(const TString& key, const NJson::TJsonValue& value) {
    AdditionalAttributes[key] = value;
    return true;
}

bool TStartrekTicket::DeserializeFromJson(const NJson::TJsonValue& data) {
    JREAD_STRING_NULLABLE_OPT(data, "description", Description);
    JREAD_STRING_NULLABLE_OPT(data, "summary", Summary);
    for (const auto& [attrName, attrValue] : data.GetMap()) {
        if (attrName != "description" && attrName != "summary") {
            SetAdditionalValue(attrName, attrValue);
        }
    }
    return true;
}

NJson::TJsonValue TStartrekTicket::SerializeToJson() const {
    NJson::TJsonValue result(NJson::JSON_MAP);
    if (!!Description) {
        JWRITE(result, "description", Description);
    }
    if (!!Summary) {
        JWRITE(result, "summary", Summary);
    }
    for (const auto& [attrName, attrValue] : AdditionalAttributes) {
        NJson::TJsonValue serializableValue;
        if (GetSerializableAdditionalAttribute(attrName, attrValue, serializableValue)) {
            JWRITE(result, attrName, std::move(serializableValue));
        }
    }
    return result;
}

bool TStartrekTicket::GetSerializableAdditionalAttribute(const TString& key, const NJson::TJsonValue& value, NJson::TJsonValue& serializableValue) const {
    if (!value.IsDefined()) {
        return false;
    }

    bool isSuccess = true;

    // filter empty tags and components
    if (key == "tags" || key == "components") {
        if (value.IsArray()) {
            serializableValue = FilterNonEmptyStrings(value);
            isSuccess = !serializableValue.GetArray().empty();
        } else if (value.IsMap()) {
            serializableValue = NJson::TJsonMap();
            for (auto&& [action, itemsToProcess] : value.GetMap()) {  // action - add/set/remove
                NJson::TJsonValue serializableItemsToProcess = FilterNonEmptyStrings(itemsToProcess);
                if (!serializableItemsToProcess.GetArray().empty()) {
                    serializableValue.InsertValue(action, std::move(serializableItemsToProcess));
                }
            }
            isSuccess = !serializableValue.GetMap().empty();
        } else {
            isSuccess = false;  // actually type is erroneous and it won't be processed correctly
        }
    } else {
        serializableValue = value;
    }

    return isSuccess;
}

bool TStartrekComment::DeserializeFromJson(const NJson::TJsonValue& info) {
    JREAD_UINT_OPT(info, "id", Id);
    JREAD_INSTANT_ISOFORMAT_NULLABLE_OPT(info, "createdAt", CreatedAt);
    JREAD_INSTANT_ISOFORMAT_NULLABLE_OPT(info, "updatedAt", UpdatedAt);
    JREAD_STRING(info, "text", Text);
    for (const auto& item : info["summonees"].GetArray()) {
        if (item["id"].IsString()) {
            Summonees.push_back(item["id"].GetString());
        } else if (item.IsString()) {
            Summonees.push_back(item.GetString());
        }
    }
    return true;
}

bool TStartrekComment::DeserializeFromString(const TString& s) {
    // treat body either as comment text or serialized comment data
    NJson::TJsonValue commentData;
    if (!(NJson::ReadJsonTree(s, &commentData, /* throwOnError = */ false) && DeserializeFromJson(commentData))) {
        SetText(s);
    }
    return true;
}

NJson::TJsonValue TStartrekComment::SerializeToJson() const {
    NJson::TJsonValue commentInfo;
    if (!!Id) {
        JWRITE(commentInfo, "id", Id);
    }
    JWRITE_INSTANT_ISOFORMAT_DEF(commentInfo, "createdAt", CreatedAt, TInstant::Zero());
    JWRITE_INSTANT_ISOFORMAT_DEF(commentInfo, "updatedAt", UpdatedAt, TInstant::Zero());
    JWRITE(commentInfo, "text", Text);
    TJsonProcessor::WriteContainerArray(commentInfo, "summonees", Summonees, /* writeEmpty = */ false);
    return commentInfo;
}

bool TStartrekTransition::DeserializeFromJson(const NJson::TJsonValue& info) {
    JREAD_STRING(info, "id", Name);
    JREAD_STRING(info, "display", DisplayName);
    if (!info.Has("to") || !info["to"].IsMap()) {
        return false;
    }
    JREAD_STRING(info["to"], "display", TargetStatusName);
    JREAD_STRING(info["to"], "key", TargetStatusKey);
    return true;
}
