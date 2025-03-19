#pragma once

#include "processor.h"
#include <util/system/mutex.h>
#include <util/system/sem.h>

namespace NCommonProxy {

    class TSource : public TProcessor {
    public:
        class TConfig : public TProcessorConfig {
        public:
            using TProcessorConfig::TProcessorConfig;
            i64 MaxInProcess = 0;
            bool UseSemaphoreForLimit = false;

        protected:
            virtual void DoToString(IOutputStream& so) const override;
            virtual void DoInit(const TYandexConfig::Section& section) override;
        };

        class TReplier : public IReplier {
        public:
            class TReporter {
            public:
                TReporter(TReplier& owner);
                virtual ~TReporter() {};
                virtual void Report(const TString& message);

            protected:
                TReplier& Owner;
            };

        public:
            TReplier(const TSource& owner, TReporter* reproter = nullptr);
            virtual void AddReply(const TString& processorName, int code = 200, const TString& message = Default<TString>(), TDataSet::TPtr data = nullptr) override final;
            virtual void AddMessage(const TString& processorName, int code, const NJson::TJsonValue& msg) override final;
            int GetCode() const {
                return CommonCode;
            }
            ~TReplier();

        protected:
            virtual void DoAddReply(const TString& processorName, int code, const TString& message, TDataSet::TPtr data);
            void SendSignals() const;
            static TString CodeToSignal(int code);

        protected:
            const TSource& Source;
            int CommonCode = 200;
            NJson::TJsonValue Message;
            TMutex Mutex;
            THolder<TReporter> Reporter;
        };

    public:
        TSource(const TString& name, const TProcessorsConfigs& configs);
        virtual const TMetaData& GetInputMetaData() const override;

    protected:
        virtual void DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const override;
        virtual void CollectInfo(NJson::TJsonValue& result) const override;
        virtual void UpdateUnistatSignals() const override;
        virtual void DoRegisterSignals(TUnistat& tass) const override;
        virtual void DoStart() override final;

    protected:
        mutable TCounter FailedCounter;
        mutable TAtomic InProcess = 0;

    private:
        const TConfig& Config;
        THolder<TFastSemaphore> MaxInProcess;
    };
}
