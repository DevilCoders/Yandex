#pragma once
#include <kernel/common_server/util/accessor.h>
#include <util/generic/fwd.h>

class TFullUserContacts {
private:
    mutable TMaybe<TString> NormalizedPhone;
    CSA_DEFAULT(TFullUserContacts, TString, FirstName);
    CSA_DEFAULT(TFullUserContacts, TString, LastName);
    CSA_DEFAULT(TFullUserContacts, TString, Patronymic);
    CSA_DEFAULT(TFullUserContacts, TString, YandexUID);
    CSA_DEFAULT(TFullUserContacts, TString, Phone);
    CSA_DEFAULT(TFullUserContacts, TString, Email);
    TString FullName;
public:
    const TString& GetFirstName(const TString& defaultValue) const {
        if (!!FirstName) {
            return FirstName;
        } else {
            return defaultValue;
        }
    }

    const TString& GetLastName(const TString& defaultValue) const {
        if (!!LastName) {
            return LastName;
        } else {
            return defaultValue;
        }
    }

    const TString& GetPatronymic(const TString& defaultValue) const {
        if (!!Patronymic) {
            return Patronymic;
        } else {
            return defaultValue;
        }
    }

    bool HasNameDetails() const;
    TFullUserContacts& SetFullName(const TString& value);
    TString GetFullName() const;
    static TString GetNormalizedPhone(const TString& phone);
    static TMaybe<TString> GetNormalizedValidPhone(const TString& phone);
    const TString& GetNormalizedPhone() const;
};

class TUserContacts {
protected:
    TString Email;
    TString PassportUid;
    TString Phone;
    TString UserId;
public:
    TUserContacts() = default;
    TUserContacts(const TString& passportUid)
        : PassportUid(passportUid)
    {
    }
    const TString& GetPhone() const {
        return Phone;
    }
    const TString& GetEmail() const {
        return Email;
    }
    const TString& GetPassportUid() const {
        return PassportUid;
    }
    ui64 GetUid() const;
    const TString& GetUserId() const {
        return UserId;
    }
    TUserContacts& SetPhone(const TString& phone) {
        Phone = phone;
        return *this;
    }
    TUserContacts& SetEmail(const TString& email) {
        Email = email;
        return *this;
    }
    TUserContacts& SetPassportUid(const TString& passportUid) {
        PassportUid = passportUid;
        return *this;
    }
    TUserContacts& SetUserId(const TString& userId) {
        UserId = userId;
        return *this;
    }
};
