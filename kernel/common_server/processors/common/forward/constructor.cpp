#include "constructor.h"

namespace NCS {
    namespace NForwardProxy {
        bool IReportConstructor::BuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const {
            TFLEventLog::Error(exceptionMessage).Signal("forward_proxy")("&code", "exception");
            return DoBuildReportOnException(request, exceptionMessage, g);
        }

        bool IReportConstructor::BuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const {
            TFLEventLog::Signal("forward_proxy")("&code", "no_reply");
            return DoBuildReportNoReply(request, g);
        }

        bool IReportConstructor::BuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const {
            TFLEventLog::Signal("forward_proxy")("&code", "success");
            return DoBuildReport(request, originalResponse, g);
        }

        bool TReportConstructorContainer::BuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const {
            if (!Object) {
                return true;
            }
            return Object->BuildReport(request, originalResponse, g);
        }

        bool TReportConstructorContainer::BuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const {
            if (Object) {
                return Object->BuildReportNoReply(request, g);
            }
            g.SetCode(HTTP_SERVICE_UNAVAILABLE);
            return true;
        }

        bool TReportConstructorContainer::BuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const {
            if (Object) {
                return Object->BuildReportOnException(request, exceptionMessage, g);
            }
            g.SetCode(HTTP_BAD_GATEWAY);
            return true;
        }

    }
}
