#pragma once

#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/auth/blackbox/info.h>

class TBlackboxAuthInfo
    : public IAuthInfo
    , protected NCS::TBlackboxInfo
{
    bool Authorized = false;
    TString ErrorMessage;

public:
    TBlackboxAuthInfo() = default;
    TBlackboxAuthInfo(const TString& errorMessage)
        : Authorized(false)
        , ErrorMessage(errorMessage)
    {
    }
    TBlackboxAuthInfo(NCS::TBlackboxInfo&& info)
        : TBlackboxInfo(std::move(info))
        , Authorized(true)
    {
    }

    virtual bool IsAvailable() const override {
        return Authorized;
    }

    virtual const TString& GetMessage() const override {
        return ErrorMessage;
    }

    virtual const TString& GetOriginatorId() const override {
        return ClientId;
    }

    const TString& GetLogin() const {
        return Login;
    }

    const TString& GetPassportUid() const {
        return PassportUid;
    }

    bool GetIsPlusUser() const {
        return IsPlusUser;
    }

    bool GetIsYandexoid() const {
        return IsYandexoid;
    }

    bool GetIgnoreDeviceId() const {
        return IgnoreDeviceId;
    }

    const TString& GetDefaultEmail() const {
        return DefaultEmail;
    }

    const TVector<TString>& GetValidatedMails() const {
        return ValidatedMails;
    }

    const TString& GetDefaultPhone() const {
        return DefaultPhone;
    }

    const TString& GetDeviceId() const {
        return DeviceId;
    }

    const TString& GetTVMTicket() const {
        return TVMTicket;
    }

    const TString& GetDeviceName() const {
        return DeviceName;
    }

    const TVector<TString>& GetScopes() const {
        return Scopes;
    }

    virtual NJson::TJsonValue GetInfo() const override;

private:
    virtual const TString& GetUserId() const override;
};
