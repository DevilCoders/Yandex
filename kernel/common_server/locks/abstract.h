#pragma once
#include <kernel/common_server/library/storage/abstract.h>

class IBaseServer;

class ILocksManager {
public:
    virtual ~ILocksManager() = default;
    virtual NRTProc::TAbstractLock::TPtr Lock(const TString& key, const bool isReadOnly, const TDuration timeout) const = 0;
};

class ILocksManagerConfig {
protected:
    virtual void DoInit(const TYandexConfig::Section* section) = 0;
    virtual void DoToString(IOutputStream& os) const = 0;
public:
    virtual ~ILocksManagerConfig() = default;
    using TPtr = TAtomicSharedPtr<ILocksManagerConfig>;
    using TFactory = NObjectFactory::TObjectFactory<ILocksManagerConfig, TString>;
    virtual TString GetType() const = 0;
    void Init(const TYandexConfig::Section* section) {
        DoInit(section);
    }
    void ToString(IOutputStream& os) const {
        DoToString(os);
    }
    virtual THolder<ILocksManager> BuildManager(const IBaseServer& server) const = 0;
};

class TLocksManagerConfigContainer {
private:
    ILocksManagerConfig::TPtr Config;
public:
    void BuildFake();

    THolder<ILocksManager> BuildManager(const IBaseServer& server) const {
        CHECK_WITH_LOG(!!Config);
        return Config->BuildManager(server);
    }

    void Init(const TYandexConfig::Section* section);

    void ToString(IOutputStream& os) const {
        if (Config) {
            os << "Type: " << Config->GetType() << Endl;
            Config->ToString(os);
        } else {
            os << "Type: " << "__unknown" << Endl;
        }
    }
};
