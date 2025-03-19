#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/library/interfaces/tvm_manager.h>
#include <kernel/common_server/library/storage/abstract/database.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/signature/abstract/abstract.h>
#include <kernel/common_server/settings/abstract/abstract.h>

#include <library/cpp/logger/global/global.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/yconf/conf.h>
#include <library/cpp/tvmauth/client/facade.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/output.h>
#include <util/system/compiler.h>

namespace NCS {

    class IPQConstructionContext {
    public:
        virtual ~IPQConstructionContext() = default;
        virtual NExternalAPI::TSender::TPtr GetSenderPtr(const TString& serviceId) const = 0;
        virtual IElSignature::TPtr GetElSignature(const TString& id) const = 0;
        virtual const ITvmManager* GetTvmManager() const = 0;
        virtual NStorage::IDatabase::TPtr GetDatabase(const TString& dbName) const = 0;
        virtual const IReadSettings& GetReadSettings() const = 0;
    };

    class TFakePQConstructionContext: public IPQConstructionContext, public IReadSettings {
    protected:
        virtual bool GetValueStr(const TString& /*key*/, TString& /*result*/) const override {
            return false;
        }
    public:
        virtual const IReadSettings& GetReadSettings() const override {
            return *this;
        }
        virtual const ITvmManager* GetTvmManager() const override {
            return nullptr;
        }
        virtual NStorage::IDatabase::TPtr GetDatabase(const TString& /*dbName*/) const override {
            return nullptr;
        }
        virtual NExternalAPI::TSender::TPtr GetSenderPtr(const TString& /*serviceId*/) const override {
            return nullptr;
        }
        virtual IElSignature::TPtr GetElSignature(const TString& /*id*/) const override {
            return nullptr;
        }
    };

    class IPQClient;

    class IPQClientConfig {
    private:
        CSA_READONLY_DEF(TString, ClientId);
    public:
        using TFactory = NObjectFactory::TParametrizedObjectFactory<IPQClientConfig, TString>;
        using TPtr = TAtomicSharedPtr<IPQClientConfig>;
    protected:
        virtual void DoInit(const TYandexConfig::Section* section) = 0;
        virtual void DoToString(IOutputStream& os) const = 0;
        virtual THolder<IPQClient> DoConstruct(const IPQConstructionContext& context) const = 0;
    public:
        virtual ~IPQClientConfig() = default;
        virtual TAtomicSharedPtr<IPQClient> Construct(const IPQConstructionContext& context) const final;
        void Init(const TYandexConfig::Section* section);
        void ToString(IOutputStream& os) const;
        virtual TString GetClassName() const = 0;
    };

    class TPQClientConfigContainer: public TBaseInterfaceContainer<IPQClientConfig> {
    private:
        using TBase = TBaseInterfaceContainer<IPQClientConfig>;
    public:
        using TBase::TBase;
    };

}
