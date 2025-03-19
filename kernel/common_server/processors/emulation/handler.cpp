#include "handler.h"

#include <kernel/common_server/server/server.h>

namespace NCS {

    void TServiceEmulationProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr /*permissions*/) {
        ReqCheckCondition(GetServer().GetEmulationsManager(), ConfigHttpStatus.ServiceUnavailable, "server not ready for this handler");
        TMaybe<NUtil::THttpReply> reply = GetServer().GetEmulationsManager()->GetHttpReply(*Context);
        ReqCheckCondition(!!reply, ConfigHttpStatus.EmptySetStatus, "no info for request");
        TString strReport = reply->Content();
        TBuffer buf(strReport.data(), strReport.size());
        for (auto&& i : reply->GetHeaders()) {
            Context->AddReplyInfo(i.Name(), i.Value());
        }
        g->SetExternalReportString(std::move(strReport), false);
        g.SetCode(reply->Code());
    }

}
