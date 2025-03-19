#pragma once

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCS {
    struct TBlackboxInfo {
        TString Login;
        TString PassportUid;
        bool IsPlusUser = false;
        bool IsYandexoid = false;
        bool IgnoreDeviceId = false;
        TString DefaultPhone;
        TString DefaultEmail;
        TString DeviceId;
        TString ClientId;
        TString DeviceName;
        TInstant Issued;
        TString TVMTicket;
        TVector<TString> Scopes;
        TVector<TString> ValidatedMails;

        TString FirstName;
        TString LastName;
        TString BirthDate;
    };
}
