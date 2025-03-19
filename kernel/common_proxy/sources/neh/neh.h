#pragma once

#include <kernel/common_proxy/common/source.h>
#include <library/cpp/neh/rpc.h>

namespace NCommonProxy {
    class TNehSource : public TSource, public NNeh::IService {
    public:
        class TConfig : public TSource::TConfig {
        public:
            using TSource::TConfig::TConfig;

            const TVector<TString>& GetListenAdresses() const {
                return ListenAddresses;
            }

            bool GetInstantReply() const {
                return InstantReply;
            }

            static TProcessorConfig::TFactory::TRegistrator<TConfig> Registrator;

        protected:
            virtual bool DoCheck() const override;
            virtual void DoInit(const TYandexConfig::Section& componentSection) override;
            virtual void DoToString(IOutputStream& so) const  override;

        protected:
            TVector<TString> ListenAddresses;
            bool InstantReply = false;
        };

    public:
        TNehSource(const TString& name, const TProcessorsConfigs& configs);
        virtual const TMetaData& GetOutputMetaData() const override;
        virtual void ServeRequest(const NNeh::IRequestRef& request) override;
        virtual void Run() override;
        static TSource::TFactory::TRegistrator<TNehSource> Registrar;

    protected:
        virtual void DoStop() override;
        virtual void DoWait() override;

    private:
        class TReplier : public TSource::TReplier {
        public:
            TReplier(const NNeh::IRequestRef& request, const TSource& source);
            virtual bool Canceled() const override;

        private:
            class TReporter : public TSource::TReplier::TReporter {
            public:
                TReporter(TSource::TReplier& owner, NNeh::IRequest* request);
                virtual void Report(const TString& message) override;

            private:
                const THolder<NNeh::IRequest> Request;
            };
            NNeh::IRequest& Request;
        };

        class TOutMetaData;

    private:
        const TConfig& Config;
        NNeh::IServicesRef Loop;
    };
}
