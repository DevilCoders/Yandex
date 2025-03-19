#include "template.h"

namespace NCS {
    namespace NForwardProxy {
        TTemplateConstructorConfig::TFactory::TRegistrator<TTemplateConstructorConfig> TTemplateConstructorConfig::Registrator(TTemplateConstructorConfig::GetTypeName());

        bool TTemplateConstructor::DoBuildReportOnException(const NNeh::THttpRequest& /*request*/, const TString& /*exceptionMessage*/, TJsonReport::TGuard& g) const {
            g.SetCode(HTTP_INTERNAL_SERVER_ERROR);
            return true;
        }

        bool TTemplateConstructor::DoBuildReportNoReply(const NNeh::THttpRequest& /*request*/, TJsonReport::TGuard& g) const {
            g.SetCode(HTTP_SERVICE_UNAVAILABLE);
            return true;
        }

        THolder<NCS::NForwardProxy::IReportConstructor> TTemplateConstructorConfig::BuildConstructor(const IBaseServer& server) const {
            return MakeHolder<TTemplateConstructor>(server);
        }

    }
}
