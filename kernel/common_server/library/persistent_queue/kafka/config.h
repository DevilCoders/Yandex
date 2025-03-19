#pragma once

#include <kernel/common_server/library/persistent_queue/abstract/config.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <contrib/libs/cppkafka/include/cppkafka/configuration.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/logger/backend_creator.h>
#include <util/folder/path.h>

namespace NCS {

    class TKafkaConfig: public IPQClientConfig {
    public:
        enum class ESecurityProtocol {
            PLAINTEXT /* "PLAINTEXT" */,
            SSL /* "SSL" */,
            SASL_PLAINTEXT /* "SASL_PLAINTEXT" */,
            SASL_SSL /* "SASL_SSL" */
        };

        class TSasl {
        public:
            enum class EMechanism {
                PLAIN /* "PLAIN" */,
                SCRAM_SHA_512 /* "SCRAM-SHA-512" */
            };
            void Init(const TYandexConfig::Section* section);
            void ToString(IOutputStream& os) const;

        public:
            CS_ACCESS(TSasl, EMechanism, Mechanism, EMechanism::PLAIN);
            CSA_DEFAULT(TSasl, TString, Username);
            CSA_DEFAULT(TSasl, TString, Password);
        };

        class TReadConfig {
            CSA_READONLY_DEF(TString, GroupId);
        public:
            TReadConfig(const TKafkaConfig& owner);
            cppkafka::Configuration BuildKafkaConfig() const;
            void Init(const TYandexConfig::Section* section);
            void ToString(IOutputStream& os) const;

        private:
            cppkafka::Configuration BaseConfig;
        };

        class TWriteConfig {
        private:
            CSA_READONLY(TString, Compression, "none");
        public:
            TWriteConfig(const TKafkaConfig& owner);
            cppkafka::Configuration BuildKafkaConfig() const;
            void Init(const TYandexConfig::Section* section);
            void ToString(IOutputStream& os) const;

        private:
            cppkafka::Configuration BaseConfig;
        };

    public:
        CS_ACCESS(TKafkaConfig, ESecurityProtocol, SecurityProtocol, ESecurityProtocol::PLAINTEXT);
        CSA_DEFAULT(TKafkaConfig, TString, Bootstrap);
        CSA_DEFAULT(TKafkaConfig, TFsPath, SslCertificate);
        CSA_READONLY_MAYBE(int, Partition);
        CSA_READONLY_DEF(TString, Topic);
        CSA_MAYBE(TKafkaConfig, TReadConfig, ReadConfig);
        CSA_MAYBE(TKafkaConfig, TWriteConfig, WriteConfig);

        virtual TString GetClassName() const override;
        static TString GetTypeName();

        static TFactory::TRegistrator<TKafkaConfig> Registrator;

    protected:
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        virtual THolder<IPQClient> DoConstruct(const IPQConstructionContext& context) const override;

    private:
        cppkafka::Configuration BuildKafkaConfig() const;

    private:
        TMaybe<TSasl> Sasl;
    };
}
