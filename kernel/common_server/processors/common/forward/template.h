#pragma once

#include "constructor.h"
#include "direct.h"

namespace NCS {
    namespace NForwardProxy {
        class TTemplateConstructorConfig: public IReportConstructorConfig {
        private:
            static TFactory::TRegistrator<TTemplateConstructorConfig> Registrator;
        protected:
            virtual bool DoInit(const TYandexConfig::Section* /*section*/) override {
                return true;
            }
            virtual void DoToString(IOutputStream& /*os*/) const override {
                return;
            }
        public:
            static TString GetTypeName() {
                return "template";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual THolder<IReportConstructor> BuildConstructor(const IBaseServer& /*server*/) const override;
        };

        class TTemplateConstructor: public TDirectConstructor {
        private:
            using TBase = TDirectConstructor;
            const IBaseServer& Server;
        protected:
            virtual bool DoBuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const override;
            virtual bool DoBuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const override;
        public:
            TTemplateConstructor(const IBaseServer& server)
                : Server(server)
            {
                Y_UNUSED(Server);
            }
        };
    }
}
