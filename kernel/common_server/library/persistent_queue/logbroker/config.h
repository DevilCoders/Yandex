#pragma once

#include <kernel/common_server/library/kikimr_auth/config.h>

#include <kernel/common_server/library/persistent_queue/abstract/config.h>
#include <kernel/common_server/util/accessor.h>
#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <kikimr/public/sdk/cpp/client/ydb_persqueue/persqueue.h>
#include <util/generic/set.h>
#include <library/cpp/logger/backend_creator.h>

namespace NCS {
    class TLogbrokerClientConfig final: public IPQClientConfig {
    public:
        class TReadConfig {
        private:
            CSA_READONLY_DEF(TSet<TString>, Topics);
            CSA_READONLY_DEF(TString, ConsumerName);

        public:
            NYdb::NPersQueue::TReadSessionSettings BuildSessionConfig() const;
            void Init(const TYandexConfig::Section* section);
            void ToString(IOutputStream& os) const;
        };
        class TWriteConfig {
        private:
            CSA_READONLY_DEF(TString, Path);
            CSA_READONLY_DEF(TString, MessagesGroupId);
            CSA_READONLY(NYdb::NPersQueue::ECodec, Codec, NYdb::NPersQueue::ECodec::GZIP);
            CSA_READONLY(ui32, MaxInFlight, 100000);
        public:
            NYdb::NPersQueue::TWriteSessionSettings BuildSessionConfig() const;
            void Init(const TYandexConfig::Section* section);
            void ToString(IOutputStream& os) const;
        };

    public:
        static TString GetTypeName();
        virtual TString GetClassName() const override;
        NYdb::TDriverConfig BuildDriverConfig() const;
        NYdb::NPersQueue::TPersQueueClientSettings BuildPQClientConfig() const;
        const TAuthConfig& GetAuthConfig() const;

        static TFactory::TRegistrator<TLogbrokerClientConfig> Registrar;

    protected:
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        virtual THolder<IPQClient> DoConstruct(const IPQConstructionContext& context) const override;

    private:
        CSA_READONLY(TString, Endpoint, "logbroker.yandex.net:2135");
        CSA_READONLY_DEF(TString, Database);
        CSA_READONLY(TDuration, InteractionTimeout, TDuration::Seconds(2));

        CSA_READONLY_MAYBE(TReadConfig, ReadConfig);
        CSA_READONLY_MAYBE(TWriteConfig, WriteConfig);
        THolder<ILogBackendCreator> Logger;
        THolder<TAuthConfig> AuthConfig;
    };
}
