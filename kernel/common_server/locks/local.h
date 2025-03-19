#pragma once
#include "abstract.h"

class TMemoryLocksManagerConfig: public ILocksManagerConfig {
private:
    static TFactory::TRegistrator<TMemoryLocksManagerConfig> Registrator;
    TString Prefix;
protected:
    void DoInit(const TYandexConfig::Section* section) override {
        Prefix = section->GetDirectives().Value("Prefix", Prefix);
    }
    void DoToString(IOutputStream& os) const override {
        os << "Prefix: " << Prefix << Endl;
    }
    virtual TString GetType() const override {
        return GetTypeName();
    }
public:

    static TString GetTypeName() {
        return "local";
    }

    virtual THolder<ILocksManager> BuildManager(const IBaseServer& server) const override;
};

class TMemoryLocksManager: public ILocksManager {
    const TString Prefix;
public:

    TMemoryLocksManager(const TString& prefix)
        : Prefix(prefix)
    {

    }
    virtual NRTProc::TAbstractLock::TPtr Lock(const TString& key, const bool isReadOnly, const TDuration timeout) const override;
};
