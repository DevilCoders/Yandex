#pragma once

#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/yconf/conf.h>

enum class EPersonalDataFields {
    Phone /* "phone" */,
    Email /* "email" */
};

class IPersonalDataStorage;

class TPersonalDataStorageConfig {
    CSA_READONLY(TString, Type, "fake");
    CSA_READONLY(TString, ClientName, "personal");
public:
    virtual ~TPersonalDataStorageConfig() {}

    virtual bool Init(const TYandexConfig::Section* section) {
        const TYandexConfig::Directives& directives = section->GetDirectives();
        Type = directives.Value("Type", Type);
        ClientName = directives.Value("ClientName", ClientName);
        return true;
    }

    virtual void ToString(IOutputStream& os) const {
        os << "Type: " << Type << Endl;
        os << "ClientName: " << ClientName << Endl;
    }

    IPersonalDataStorage* BuildStorage(IBaseServer& server) const;
};

class IPersonalDataStorage {
public:
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IPersonalDataStorage, TString, TPersonalDataStorageConfig, const IBaseServer&>;
    using TPtr = TAtomicSharedPtr<IPersonalDataStorage>;

    virtual TString GetOrCreateAlias(const TString& userId, const EPersonalDataFields field, const TString& original) const = 0;

    virtual TString RestoreData(const EPersonalDataFields field, const TString& alias) const = 0;

    virtual ~IPersonalDataStorage() {}
};


class TFakePersonalDataStorage : public IPersonalDataStorage {
    static TFactory::TRegistrator <TFakePersonalDataStorage> Registrator;
public:
    TFakePersonalDataStorage(const TPersonalDataStorageConfig& /*config*/, const IBaseServer& /* server */) {}

    virtual TString GetOrCreateAlias(const TString& /*userId*/, const EPersonalDataFields /*field*/, const TString& original) const override {
        return original;
    }

    virtual TString RestoreData(const EPersonalDataFields /*field*/, const TString& alias) const override {
        return alias;
    }

};
