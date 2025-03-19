#pragma once

#include <kernel/common_proxy/common/source.h>
#include <util/system/event.h>

namespace NCommonProxy {
    class TLimitedSource : public TSource {
    public:
        class TConfig : public TSource::TConfig {
        public:
            using TSource::TConfig::TConfig;

            struct TAlert {
                double SuccessRatio = 0;
                TDuration Period;
                void Init(const TYandexConfig::Section& section);
                void ToString(IOutputStream& so) const;
            };

        public:
            TVector<TAlert> Alerts;
            ui32 CheckAlertsPeriod = 1000;

        protected:
            virtual bool DoCheck() const override;
            virtual void DoInit(const TYandexConfig::Section& componentSection) override;
            virtual void DoToString(IOutputStream& so) const override;
        };

        class TReplier : public TSource::TReplier {
        public:
            TReplier(const TLimitedSource& owner);
            ~TReplier();

        private:
            const TLimitedSource& Owner;
        };

    public:
        TLimitedSource(const TString& name, const TProcessorsConfigs& configs);
        virtual void Run() override final;

    protected:
        virtual void DoStop() override;
        virtual void DoWait() override;
        virtual void DoRun() = 0;
        virtual void DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const override;

    protected:
        volatile bool Stopped;

    private:
        void CheckAlerts(bool periodic) const;
        virtual void Report() const;

    private:
        const TConfig& Config;
        TManualEvent Finished;
    };
}
