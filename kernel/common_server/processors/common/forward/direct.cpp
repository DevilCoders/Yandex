#include "direct.h"

namespace NCS {
    namespace NForwardProxy {
        TDirectConstructorConfig::TFactory::TRegistrator<TDirectConstructorConfig> TDirectConstructorConfig::Registrator(TDirectConstructorConfig::GetTypeName());

        bool TDirectConstructor::DoBuildReport(const NNeh::THttpRequest& /*request*/, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const {
            TString contentCopy = originalResponse.Content();
            g.SetExternalReportString(std::move(contentCopy), false);
            g.SetCode(originalResponse.Code());
            auto context = g->GetContextPtr();
            const ci_equal_to comparator;
            for (auto&& h : originalResponse.GetHeaders()) {
                if (comparator(h.Name(), "content-length")) {
                    continue;
                }
                context->AddReplyInfo(h.Name(), h.Value());
            }
            return true;
        }

        bool TDirectConstructor::DoBuildReportOnException(const NNeh::THttpRequest& /*request*/, const TString& /*exceptionMessage*/, TJsonReport::TGuard& g) const {
            g.SetCode(HTTP_INTERNAL_SERVER_ERROR);
            return true;
        }

        bool TDirectConstructor::DoBuildReportNoReply(const NNeh::THttpRequest& /*request*/, TJsonReport::TGuard& g) const {
            g.SetCode(HTTP_SERVICE_UNAVAILABLE);
            return true;
        }

        THolder<NCS::NForwardProxy::IReportConstructor> TDirectConstructorConfig::BuildConstructor(const IBaseServer& /*server*/) const {
            return MakeHolder<TDirectConstructor>();
        }

    }
}
