#pragma once
#include <kernel/common_server/report/json.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/util/network/neh_request.h>
#include "constructor.h"

namespace NCS {
    namespace NForwardProxy {
        class TAgent {
        private:
            NExternalAPI::TSender::TPtr Sender;
            TReportConstructorContainer ConstructorContainer;
        public:
            TAgent(NExternalAPI::TSender::TPtr sender, TReportConstructorContainer constructorContainer = TReportConstructorContainer())
                : Sender(sender)
                , ConstructorContainer(constructorContainer)
            {

            }
            void ForwardToExternalAPI(TJsonReport::TGuard& g, const NNeh::THttpRequest& request);
        };

    }
}
