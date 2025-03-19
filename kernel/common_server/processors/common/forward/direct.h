#pragma once

#include "constructor.h"

namespace NCS {
    namespace NForwardProxy {
        class TDirectConstructorConfig: public IReportConstructorConfig {
        private:
            static TFactory::TRegistrator<TDirectConstructorConfig> Registrator;
        protected:
            virtual bool DoInit(const TYandexConfig::Section* /*section*/) override {
                return true;
            }
            virtual void DoToString(IOutputStream& /*os*/) const override {
                return;
            }
        public:
            static TString GetTypeName() {
                return "direct";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual THolder<IReportConstructor> BuildConstructor(const IBaseServer& /*server*/) const override;
        };

        class TDirectConstructor: public IReportConstructor {
        protected:
            virtual bool DoBuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const override;
            virtual bool DoBuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const override;
            virtual bool DoBuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const override;
        public:

        };
    }
}
