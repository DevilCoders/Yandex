#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/util/network/neh_request.h>
#include <library/cpp/json/json_value.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/builder.h>
#include <util/generic/serialized_enum.h>

class TStaffEntry {
public:
    enum class EStaffEntryField {
        Id /* "id" */,
        Uid /* "uid" */,
        Username /* "login" */,
        Name /* "name" */,
        WorkPhone /* "work_phone" */,
        WorkEmail /* "work_email" */,
        MobilePhone /* "phones" */,
        IsDeleted /* "is_deleted" */,
        IsDismissed /* "official.is_dismissed" */,
        QuitAt /* "official.quit_at" */,
        DepartmentUrl /* "department_group.url" */,
        GroupId /* "groups.group.id" */,
        GroupName /* "groups.group.name" */
    };

    using TStaffEntryFields = TSet<EStaffEntryField>;

    enum class EStaffNameLocale {
        EN /* "en" */,
        RU /* "ru" */
    };

    class TStaffName {
        RTLINE_READONLY_ACCEPTOR_DEF(FirstName, TString);
        RTLINE_READONLY_ACCEPTOR_DEF(LastName, TString);
        RTLINE_READONLY_ACCEPTOR_DEF(MiddleName, TString);

    public:
        bool DeserializeFromJson(const NJson::TJsonValue& data, EStaffNameLocale locale) {
            FirstName = ReadNameFieldSafe(data, "first", locale);
            LastName = ReadNameFieldSafe(data, "last", locale);
            MiddleName = ReadNameFieldSafe(data, "middle", locale);
            return true;
        }

    private:
        TString ReadNameFieldSafe(const NJson::TJsonValue& data, const TString& name, EStaffNameLocale locale) const {
            TString result = "";
            if (data.IsMap() && data.Has(name)) {
                auto localizedNames = data[name].GetMap();
                auto needle = localizedNames.FindPtr(::ToString(locale));
                if (needle && needle->IsString()) {
                    result = Strip(needle->GetString());
                }
            }
            return result;
        }
    };

    class TStaffGroup {
        RTLINE_READONLY_ACCEPTOR(Id, ui64, 0);
        RTLINE_READONLY_ACCEPTOR_DEF(Name, TString);
    public:
        bool DeserializeFromJson(const NJson::TJsonValue& data) {
            const auto& group = data["group"];
            Id = group["id"].GetUIntegerRobust();
            Name = group["name"].GetString();
            return true;
        }
    };

    using TStaffNames = TMap<EStaffNameLocale, TStaffName>;

    RTLINE_READONLY_ACCEPTOR(Id, ui64, 0);
    RTLINE_READONLY_ACCEPTOR(Uid, ui64, 0);
    RTLINE_READONLY_ACCEPTOR_DEF(Username, TString);
    RTLINE_READONLY_ACCEPTOR_DEF(Name, TStaffNames);
    RTLINE_READONLY_ACCEPTOR(WorkPhone, ui64, 0);
    RTLINE_READONLY_ACCEPTOR_DEF(WorkEmail, TString);
    RTLINE_READONLY_ACCEPTOR_DEF(MainMobilePhone, TString);
    RTLINE_READONLY_ACCEPTOR_DEF(MobilePhones, TVector<TString>);
    RTLINE_READONLY_FLAG_ACCEPTOR(Deleted, false);
    RTLINE_READONLY_FLAG_ACCEPTOR(Dismissed, false);
    RTLINE_READONLY_ACCEPTOR(QuitAt, TInstant, TInstant::Zero());
    RTLINE_READONLY_ACCEPTOR_DEF(DepartmentUrl, TString);
    RTLINE_READONLY_ACCEPTOR_DEF(Groups, TVector<TStaffGroup>);

public:
    TStaffEntry(const TStaffEntryFields& fields = {})
        : Fields(fields)
    {
    }

    bool DeserializeFromJson(const NJson::TJsonValue& data);

    bool HasField(EStaffEntryField field) const {
        return !Fields || Fields.contains(field);
    }

    bool HasOneOf(const TStaffEntryFields& fields) const {
        if (!Fields) {
            return true;
        }
        for (const auto f: fields) {
            if (HasField(f)) {
                return true;
            }
        }
        return false;
    }

    TString GetFieldsStr() const {
        if (!!Fields) {
            auto f = Fields;
            f.emplace(EStaffEntryField::Id);
            return JoinSeq(",", f);
        }

        TVector<TString> fields;
        for (const auto& [_, name] : GetEnumNames<EStaffEntryField>()) {
            fields.emplace_back(name);
        }
        return JoinSeq(",", fields);
    }

private:
    inline bool GetNestedField(const NJson::TJsonValue& data, const TString& name, const TString& subname, TString& result) const;
    inline bool GetNestedField(const NJson::TJsonValue& data, const TString& name, const TString& subname, bool& result) const;
    inline bool GetNestedField(const NJson::TJsonValue& data, const TString& name, const TString& subname, NJson::TJsonValue& result) const;

    TStaffEntryFields Fields;
};


class TStaffEntrySelector {
public:
    enum class EStaffEntryField {
        Uid /* "uid" */,
        Username /* "login" */,
        WorkPhone /* "work_phone" */,
        WorkEmail /* "work_email" */,
        DepartmentUrl /* "department_url" */
    };

    RTLINE_READONLY_ACCEPTOR_DEF(Type, EStaffEntryField);
    RTLINE_READONLY_ACCEPTOR_DEF(Value, TString);

public:
    TStaffEntrySelector(EStaffEntryField type, const TString& value)
        : Type(type)
        , Value(value)
    {
    }

    template <typename TContainer>
    TStaffEntrySelector(EStaffEntryField type, const TContainer& values)
        : TStaffEntrySelector(type, JoinSeq(",", values))
    {
    }

    void FillRequest(NNeh::THttpRequest& request, TMaybe<ui64> firstId) const;
};
