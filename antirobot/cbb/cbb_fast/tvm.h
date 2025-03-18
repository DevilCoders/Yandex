#pragma once


#include <library/cpp/tvmauth/client/facade.h>


namespace NCbbFast {

class TConfig;

class TInvalidTvmClientException : public yexception {
};

const TString X_YA_SERVICE_TICKET = "X-Ya-Service-Ticket";

struct TTvmClientInfo {
    NTvmAuth::TTvmId ClientId;
    TString ClientName;
};

class TTvm {
    THolder<NTvmAuth::TTvmClient> TvmClient;
    THashMap<NTvmAuth::TTvmId, TTvmClientInfo> AllowedClients;

public:
    explicit TTvm(const NCbbFast::TConfig& config);
    TTvmClientInfo CheckClientTicket(TStringBuf ticket) const;
};

} // namespace NCbbFast
