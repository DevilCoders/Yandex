#pragma once
#include "abstract.h"

class TFakeLocksManagerConfig: public ILocksManagerConfig {
private:
    static TFactory::TRegistrator<TFakeLocksManagerConfig> Registrator;
    bool MakeRealObjects = false;
protected:
    void DoInit(const TYandexConfig::Section* section) override {
        MakeRealObjects = section->GetDirectives().Value("MakeRealObjects", MakeRealObjects);
    }
    void DoToString(IOutputStream& os) const override {
        os << "MakeRealObjects: " << MakeRealObjects << Endl;
    }
    virtual TString GetType() const override {
        return GetTypeName();
    }
public:

    static TString GetTypeName() {
        return "fake";
    }

    virtual THolder<ILocksManager> BuildManager(const IBaseServer& server) const override;
};

class TFakeLocksManager: public ILocksManager {
private:
    bool MakeRealObjects = false;
public:
    TFakeLocksManager(const bool makeRealObjects)
        : MakeRealObjects(makeRealObjects)
    {

    }

    virtual NRTProc::TAbstractLock::TPtr Lock(const TString& key, const bool isReadOnly, const TDuration timeout) const override;
};
