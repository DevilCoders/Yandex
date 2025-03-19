#pragma once
#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <contrib/libs/cxxsupp/libcxx/include/string>
#include <library/cpp/cache/cache.h>
#include <util/generic/singleton.h>
#include <util/system/mutex.h>


enum class EApikeyPassStatus {
    OK = 0 /* "ok" */,
    Reject = 1 /* "reject" */,
    OKCache = 2 /* "ok_cache" */,
    RejectCache = 3 /* "reject_cache" */,
    OK5xx = 4 /* "ok_5xx" */
};

class TApikey2AuthInfo: public IAuthInfo {
    bool Authorized;
    TString UserId;
public:
    TApikey2AuthInfo()
        : Authorized(false)
    {}

    TApikey2AuthInfo(const bool authorized, const TString& userId)
        : Authorized(authorized)
        , UserId(userId)
    {
    }

    virtual bool IsAvailable() const override {
        return Authorized;
    }

    virtual const TString& GetUserId() const override {
        return UserId;
    }
};

class TApikey2AuthConfig: public IAuthModuleConfig {
private:
    static TFactory::TRegistrator<TApikey2AuthConfig> Registrator;

    RTLINE_READONLY_ACCEPTOR(Url, TString, "");
    RTLINE_READONLY_ACCEPTOR(AdditionalCgi, TString, "");
    RTLINE_READONLY_ACCEPTOR(KeyParameter, TString, "apikey");
    RTLINE_READONLY_ACCEPTOR(ServiceKeyParameter, TString, "key");
    RTLINE_READONLY_ACCEPTOR(PassOn5xx, bool, true);
    RTLINE_READONLY_ACCEPTOR(CacheSize, ui32, 100);
    RTLINE_READONLY_ACCEPTOR(CacheAge, TDuration, TDuration::Seconds(30));
protected:

    virtual void DoInit(const TYandexConfig::Section* section) override {
        AssertCorrectConfig(section->GetDirectives().GetValue("Url", Url), "no 'Url' field");
        AdditionalCgi = section->GetDirectives().Value("AdditionalCgi", AdditionalCgi);
        KeyParameter = section->GetDirectives().Value("KeyParameter", KeyParameter);
        AssertCorrectConfig(!!KeyParameter, "empty 'ParameterName' field");
        ServiceKeyParameter = section->GetDirectives().Value("ServiceKeyParameter", ServiceKeyParameter);
        AssertCorrectConfig(!!ServiceKeyParameter, "empty 'ServiceKeyParameter' field");
        PassOn5xx = section->GetDirectives().Value("PassOn5xx", PassOn5xx);
        CacheSize = section->GetDirectives().Value("CacheSize", CacheSize);
        CacheAge = section->GetDirectives().Value("CacheAge", CacheAge);
    }

    virtual void DoToString(IOutputStream& os) const override {
        os << "Url: " << Url << Endl;
        os << "AdditionalCgi: " << AdditionalCgi << Endl;
        os << "KeyParameter: " << KeyParameter << Endl;
        os << "ServiceKeyParameter: " << ServiceKeyParameter << Endl;
        os << "PassOn5xx: " << PassOn5xx << Endl;
        os << "CacheSize: " << CacheSize << Endl;
        os << "CacheAge: " << CacheAge << Endl;
    }
    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* /*server*/) const override;
public:

    TApikey2AuthConfig(const TString& name)
        : IAuthModuleConfig(name)
    {
    }

};

class TApikey2AuthModule: public IAuthModule {
public:
    struct TCacheInfo {
        bool Authorized;
        TInstant RequestTime = TInstant::Zero();

        TCacheInfo() {}
        TCacheInfo(bool authorized, TInstant requestTime)
            : Authorized(authorized)
            , RequestTime(requestTime)
        {}
    };

    class TCache {
    private:
        using TKeysCache = TLRUCache<TString, TCacheInfo>;
        mutable TRWMutex Mutex;
        const size_t Size = 100;
        const TDuration Age = TDuration::Seconds(30);

    public:
        TCache()
        {
        }

        TCache(size_t size, TDuration age)
            : Size(size)
            , Age(age)
        {
        }

        bool Find(const TString& apikey, TCacheInfo& info) {
            TReadGuard g(Mutex);
            auto cache = Singleton<TKeysCache>(Size);
            auto cached = cache->Find(apikey);
            if (cached != cache->End() && cached.Value().RequestTime + Age >= Now()) {
                info = cached.Value();
                return true;
            }
            return false;
        }

        void Update(const TString& apikey, bool authorized) {
            TWriteGuard g(Mutex);
            Singleton<TKeysCache>(Size)->Update(apikey, TCacheInfo(authorized, Now()));
        }
    };

private:
    const TApikey2AuthConfig* Config = nullptr;
    mutable TCache Cache;
    TEnumSignal<EApikeyPassStatus, double> ApikeyStatusSignal;

    TString BuildAuthRequest(IReplyContext::TPtr requestContext, TString& authToken) const;
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const override;
public:
    TApikey2AuthModule(const TApikey2AuthConfig* config)
        : IAuthModule()
        , Config(config)
        , Cache(Config->GetCacheSize(), Config->GetCacheAge())
        , ApikeyStatusSignal({ "frontend-apikey_status" }, false)
    {
    }

};


