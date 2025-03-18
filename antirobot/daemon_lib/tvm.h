#pragma once

#include <library/cpp/tvmauth/client/facade.h>

namespace NAntiRobot {
    const TString TVM_SECRET_ENV = "ANTIROBOT_TVM_SECRET";
    const TString X_YA_SERVICE_TICKET = "X-Ya-Service-Ticket";

    struct TAntirobotTvmClientInfo {
        NTvmAuth::TTvmId ClientId;
        TString ClientName;
    };

    class TInvalidTvmClientException : public yexception {
    };

    class TAntirobotTvm {
        THolder<NTvmAuth::TTvmClient> TvmClient;
        THashMap<NTvmAuth::TTvmId, TAntirobotTvmClientInfo> AllowedClients;

    public:
        TAntirobotTvm(bool useTvmClient, const TString& clientsList);
        TString GetServiceTicket(NTvmAuth::TTvmId targetClientId) const;
        TAntirobotTvmClientInfo CheckClientTicket(TStringBuf ticket) const;
    };
}
