#include "entry.h"

bool TStaffEntry::DeserializeFromJson(const NJson::TJsonValue& data) {
    Id = data["id"].GetUIntegerRobust();
    if (HasField(EStaffEntryField::Uid)) {
        TString uidStr;
        JREAD_STRING(data, "uid", uidStr);
        Uid = FromString<decltype(Uid)>(uidStr);
    }

    if (HasField(EStaffEntryField::Username)) {
        JREAD_STRING(data, "login", Username);
    }

    if (HasField(EStaffEntryField::Name)) {
        if (!data.Has("name") || !data["name"].IsMap()) {
            return false;
        }
        for (const auto& [locale, _] : GetEnumNames<EStaffNameLocale>()) {
            TStaffName localizedName;
            if (!localizedName.DeserializeFromJson(data["name"], locale)) {
                return false;
            }
            Name.emplace(locale, std::move(localizedName));
        }
    }

    if (HasField(EStaffEntryField::WorkPhone)) {
        JREAD_UINT_NULLABLE_OPT(data, "work_phone", WorkPhone);
    }

    if (HasField(EStaffEntryField::WorkEmail)) {
        JREAD_STRING_NULLABLE_OPT(data, "work_email", WorkEmail);
    }

    if (HasField(EStaffEntryField::MobilePhone)) {
        for (const auto& phone : data["phones"].GetArray()) {
            TString phoneNumber;
            JREAD_STRING_NULLABLE_OPT(phone, "number", phoneNumber);
            MobilePhones.push_back(phoneNumber);
            if (phone["is_main"].GetBoolean()) {
                MainMobilePhone = phoneNumber;
            }
        }
    }

    if (HasField(EStaffEntryField::IsDeleted)) {
        JREAD_BOOL(data, "is_deleted", DeletedFlag);
    }

    if (HasField(EStaffEntryField::IsDismissed) && !GetNestedField(data, "official", "is_dismissed", DismissedFlag)) {
        return false;
    }

    if (HasField(EStaffEntryField::QuitAt)) {
        TString quitAtStr;
        if (GetNestedField(data, "official", "quit_at", quitAtStr)) {
            if (!TInstant::TryParseIso8601(quitAtStr, QuitAt)) {
                return false;
            }
        }  /* else it's null - OK then */
    }

    if (HasField(EStaffEntryField::DepartmentUrl) && !GetNestedField(data, "department_group", "url", DepartmentUrl)) {
        return false;
    }

    if (HasOneOf({ EStaffEntryField::GroupId, EStaffEntryField::GroupName })) {
        for (const auto& g: data["groups"].GetArray()) {
            Groups.emplace_back();
            if (!Groups.back().DeserializeFromJson(g)) {
                return true;
            }
        }
    }

    return true;
}

bool TStaffEntry::GetNestedField(const NJson::TJsonValue& data, const TString& name, const TString& subname, TString& result) const {
    NJson::TJsonValue rawResult;
    if (GetNestedField(data, name, subname, rawResult) && rawResult.IsString()) {
        result = rawResult.GetString();
        return true;
    }
    return false;
}

bool TStaffEntry::GetNestedField(const NJson::TJsonValue& data, const TString& name, const TString& subname, bool& result) const {
    NJson::TJsonValue rawResult;
    if (GetNestedField(data, name, subname, rawResult) && rawResult.IsBoolean()) {
        result = rawResult.GetBoolean();
        return true;
    }
    return false;
}

bool TStaffEntry::GetNestedField(const NJson::TJsonValue& data, const TString& name, const TString& subname, NJson::TJsonValue& result) const {
    if (data.Has(name) && data[name].IsMap()) {
        auto field = data[name].GetMap();
        auto fieldValue = field.FindPtr(subname);
        if (fieldValue) {
            result = *fieldValue;
            return true;
        }
    }
    return false;
}

void TStaffEntrySelector::FillRequest(NNeh::THttpRequest& request, TMaybe<ui64> firstId) const {
    request.SetUri("/v3/persons/");
    TStringBuilder q;
    if (!!firstId) {
        q << "id>" << *firstId;
    }
    if (Type == EStaffEntryField::DepartmentUrl) {
        q << " and (department_group.department.url==\"" << Value << "\" or "
          << "department_group.ancestors.department.url==\"" << Value << "\")";
    } else {
        request.AddCgiData(::ToString(Type), Value);
    }
    if (!q.empty()) {
        request.AddCgiData("_query", q);
    }
}
