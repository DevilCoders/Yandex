#include "handler.h"

namespace NCS {

    void TMiracleHandler::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr /*permissions*/) {
        NNeh::THttpRequest incomeRequest;
        incomeRequest.SetUri(Context->GetUri());
        incomeRequest.SetCgiData(Context->GetCgiParameters().Print());
        if (!GetRawData().Empty()) {
            incomeRequest.SetPostData(GetRawData().AsStringBuf());
        }
        for (const auto& [key, value] : Context->GetBaseRequestData().HeadersIn()) {
            incomeRequest.AddHeader(key, value);
        }
        g.MutableReport().AddReportElement("income_request", incomeRequest.SerializeToJson());
    }

}
