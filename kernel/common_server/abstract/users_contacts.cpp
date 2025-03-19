#include "users_contacts.h"
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/generic/vector.h>

bool TFullUserContacts::HasNameDetails() const {
    return !!FirstName || !!LastName || !!Patronymic;
}

TFullUserContacts& TFullUserContacts::SetFullName(const TString& value) {
    TVector<TString> elements;
    StringSplitter(value).SplitBySet(" ").SkipEmpty().Collect(&elements);
    if (elements.size() == 3) {
        LastName = elements[0];
        FirstName = elements[1];
        Patronymic = elements[2];
    } else {
        FullName = value;
    }
    return *this;
}

TString TFullUserContacts::GetFullName() const {
    if (!!FullName) {
        return FullName;
    } else {
        return JoinSeq(" ", { LastName, FirstName, Patronymic });
    }
}

TString TFullUserContacts::GetNormalizedPhone(const TString& phone) {
    TString normalizedPhone;
    for (auto&& c : phone) {
        if (c >= '0' && c <= '9') {
            normalizedPhone += c;
        }
    }
    if (normalizedPhone.size() == 10) {
        return "+7" + normalizedPhone;
    } else if (normalizedPhone.StartsWith("8") || normalizedPhone.StartsWith("9")) {
        return "+7" + normalizedPhone.substr(1);
    } else {
        return "+" + normalizedPhone;
    }
}

const TString& TFullUserContacts::GetNormalizedPhone() const {
    if (!NormalizedPhone) {
        NormalizedPhone = GetNormalizedPhone(Phone);
    }
    return *NormalizedPhone;
}

TMaybe<TString> TFullUserContacts::GetNormalizedValidPhone(const TString& phone) {
    TString normalizedPhone = GetNormalizedPhone(phone);
    if (!normalizedPhone.StartsWith("+7")) {
        return Nothing();
    }
    if (normalizedPhone.size() != 12) {
        return Nothing();
    }
    return normalizedPhone;
}

ui64 TUserContacts::GetUid() const {
    ui64 result = 0;
    TryFromString(PassportUid, result);
    return result;
}
