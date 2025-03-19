#pragma once

#include <kernel/common_server/library/interfaces/tvm_manager.h>
#include <kernel/common_server/util/accessor.h>
#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <library/cpp/yconf/conf.h>

namespace NCS {

    class TAuthConfig {
    public:
        enum class EMethod {
            None,
            OAuth,
            Tvm
        };
        virtual ~TAuthConfig() = default;
        TAuthConfig(EMethod method);
        static THolder<TAuthConfig> Create(const TYandexConfig::Section* section);
        void ToString(IOutputStream& os) const;
        virtual bool PatchDriverConfig(NYdb::TDriverConfig& driverConfig, const ITvmManager* tvmManager) const = 0;
        virtual void Init(const TYandexConfig::Section* section) = 0;

    protected:
        virtual void DoToString(IOutputStream& os) const = 0;
        CSA_READONLY_DEF(EMethod, Method);
    };

    class TAuthConfigNone : public TAuthConfig {
    public:
        TAuthConfigNone();
        virtual bool PatchDriverConfig(NYdb::TDriverConfig& /*driverConfig*/, const ITvmManager* /*tvmManager*/) const override;
        virtual void Init(const TYandexConfig::Section* /*section*/) override;
        virtual void DoToString(IOutputStream& /*os*/) const override;
    };

    class TAuthConfigOAuth : public TAuthConfig {
    public:
        TAuthConfigOAuth();
        virtual bool PatchDriverConfig(NYdb::TDriverConfig& driverConfig, const ITvmManager* /*tvmManager*/) const override;
        virtual void Init(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        CSA_DEFAULT(TAuthConfigOAuth, TString, Token);
    };

    class TAuthConfigTvm : public TAuthConfig {
    public:
        TAuthConfigTvm();
        virtual bool PatchDriverConfig(NYdb::TDriverConfig& driverConfig, const ITvmManager* tvmManager) const override;
        virtual void Init(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        CSA_DEFAULT(TAuthConfigTvm, TString, TvmClientName);
        CS_ACCESS(TAuthConfigTvm, TString, DestinationAlias, "logbroker");
    };

}
