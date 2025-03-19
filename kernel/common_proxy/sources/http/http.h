#pragma once

#include <kernel/common_proxy/common/source.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/http_ex.h>

namespace NCommonProxy {
    class THttpSource : public TSource, public THttpServer::ICallBack {
    public:
        class TConfig : public TSource::TConfig {
        public:
            using TSource::TConfig::TConfig;

            const THttpServer::TOptions& GetHttpOptions() const;
            static TProcessorConfig::TFactory::TRegistrator<TConfig> Registrator;

        protected:
            virtual bool DoCheck() const override;
            virtual void DoInit(const TYandexConfig::Section& componentSection) override;
            virtual void DoToString(IOutputStream& so) const  override;

        private:
            TDaemonConfig::THttpOptions HttpOptions;
        };

    public:
        THttpSource(const TString& name, const TProcessorsConfigs& configs);
        virtual TClientRequest* CreateClient() override;
        virtual const TMetaData& GetOutputMetaData() const override;
        virtual void Run() override;
        static TSource::TFactory::TRegistrator<THttpSource> Registrar;

    protected:
        virtual void DoStop() override;
        virtual void DoWait() override;
        virtual void CollectInfo(NJson::TJsonValue& result) const override;

    private:
        class TClient : public THttpClientRequestEx {
        public:
            TClient(const TSource& source);
            ~TClient();
            virtual bool Reply(void* /*ThreadSpecificResource*/) override;
        private:
            const TSource& Source;
        };

        class TReplier : public TSource::TReplier {
        public:
            TReplier(TClient* client, const TSource& source);

        private:
            class TReporter : public TSource::TReplier::TReporter {
            public:
                TReporter(TSource::TReplier& owner, TClient* client);
                virtual void Report(const TString& message) override;

            private:
                THolder<TClient> Client;
            };
        };

        class TOutMetaData;

    private:
        THttpServer Server;
    };
}
