#include "agent.h"
#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>

namespace NCS {
    namespace NForwardProxy {

        void TAgent::ForwardToExternalAPI(TJsonReport::TGuard& g, const NNeh::THttpRequest& request) {
            if (!Sender) {
                g.AddReportElement("reason", "no sender");
                g.SetCode(HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
            NExternalAPI::TServiceApiHttpDirectRequest fakeReq(request);
            const auto f = Sender->SendRequestAsync(fakeReq);

            auto frBuilder = g.Release();
            const TString externalApiName = Sender->GetApiName();
            auto constructorContainerCopy = ConstructorContainer;
            auto localRequest = request;
            f.Subscribe(
                [frBuilder, externalApiName, constructorContainerCopy, localRequest](const NThreading::TFuture<NExternalAPI::TServiceApiHttpDirectRequest::TResponse>& resultFuture) {
                    TJsonReport::TGuard g(frBuilder, HTTP_INTERNAL_SERVER_ERROR);
                    auto context = g->GetContextPtr();
                    context->AddReplyInfo("ForwardExternalAPI", externalApiName);
                    if (resultFuture.HasValue()) {
                        constructorContainerCopy.BuildReport(localRequest, resultFuture.GetValue().GetReply(), g);
                        return;
                    } else if (resultFuture.HasException()) {
                        try {
                            resultFuture.GetValue();
                        } catch (...) {
                            constructorContainerCopy.BuildReportOnException(localRequest, CurrentExceptionMessage(), g);
                            return;
                        }
                    }
                    constructorContainerCopy.BuildReportNoReply(localRequest, g);
                }
            );
        }

    }
}
