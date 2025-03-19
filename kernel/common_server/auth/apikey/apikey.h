#pragma once
#include <kernel/common_server/auth/common/auth.h>
#include <library/cpp/cgiparam/cgiparam.h>


class TApikeyAuthInfo: public IAuthInfo {
private:
    bool Authorized;
    TString Apikey;
    int HttpCode;
    TString Message;

    static const TString Name;
public:
    TApikeyAuthInfo(bool authorized, const TString& apikey, int httpCode, const TString& message)
        : Authorized(authorized)
        , Apikey(apikey)
        , HttpCode(httpCode)
        , Message(message)
    {
    }

    virtual bool IsAvailable() const override {
        return Authorized;
    }

    virtual const TString& GetUserId() const override {
        return Apikey;
    }

    int GetHttpCode() const {
        return HttpCode;
    }

    const TString& GetMessage() const override {
        return Message;
    }

    int GetCode() const override {
        return HttpCode;
    }
};

class TApikeyAuthConfig;

class TApikeyAuthModule: public IAuthModule {
private:
    const TApikeyAuthConfig* Config = nullptr;
protected:
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const;
public:

    TApikeyAuthModule(const TApikeyAuthConfig* config)
        : Config(config)
    {
    }

    TString GetObfuscatedKey(const TString& key) const;
    IAuthInfo::TPtr CheckApikey(const TCgiParameters& cgi) const;
};

class TApikeyAuthConfig: public IAuthModuleConfig {
private:
    static TFactory::TRegistrator<TApikeyAuthConfig> Registrator;
    static TSet<TString> RequiredModules;

    TString KeyParameter;
    bool StrictMode;
    bool ObfuscateKey;
protected:
    virtual void DoInit(const TYandexConfig::Section* section) override {
        KeyParameter = section->GetDirectives().Value("KeyParameter", KeyParameter);
        AssertCorrectConfig(!!KeyParameter, "no 'KeyParameter' field");
        StrictMode = section->GetDirectives().Value("StrictMode", StrictMode);
        ObfuscateKey = section->GetDirectives().Value("ObfuscateKey", ObfuscateKey);
    }

    virtual void DoToString(IOutputStream& os) const override {
        os << "KeyParameter: " << KeyParameter << Endl;
        os << "StrictMode: " << StrictMode << Endl;
        os << "ObfuscateKey: " << ObfuscateKey << Endl;
    }

    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* /*server*/) const override {
        return MakeHolder<TApikeyAuthModule>(this);
    }

public:

    TApikeyAuthConfig(const TString& name)
        : IAuthModuleConfig(name)
        , KeyParameter("apikey")
        , StrictMode(true)
        , ObfuscateKey(false)
    {

    }

    const TString& GetKeyParameter() const {
        return KeyParameter;
    }

    bool GetStrictMode() const {
        return StrictMode;
    }

    bool GetObfuscateKey() const {
        return ObfuscateKey;
    }

    const TSet<TString>& GetRequiredModules() const override {
        return RequiredModules;
    }
};
