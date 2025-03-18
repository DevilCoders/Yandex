#include "tvm.h"
#include "config_global.h"
#include "eventlog_err.h"

#include <util/folder/dirut.h>
#include <util/system/env.h>

using namespace NAntiRobot;


TAntirobotTvm::TAntirobotTvm(bool useTvmClient, const TString& clientsList) {
    if (!useTvmClient) {
        return;
    }

    if (clientsList) {
        TFileInput confFile(clientsList);
        NJson::TJsonValue jsonValue;
        ReadJsonTree(confFile.ReadAll(), &jsonValue, true);
        for (const auto& client : jsonValue["clients"].GetArraySafe()) {
            NTvmAuth::TTvmId id = client["client_id"].GetUIntegerSafe();
            AllowedClients[id] = {
                id,
                client["name"].GetStringSafe(),
            };
        }
    }

    const TString tvmSecret = GetEnv(TVM_SECRET_ENV);
    if (tvmSecret.empty()) {
        ythrow yexception() << "Fatal: could not get TVM secret. Environment variable '" << TVM_SECRET_ENV << "' is not defined";
    }

    MakeDirIfNotExist(ANTIROBOT_DAEMON_CONFIG.TVMClientCacheDir);

    NTvmAuth::NTvmApi::TClientSettings settings;

    settings.SetSelfTvmId(ANTIROBOT_DAEMON_CONFIG.AntirobotTVMClientId);
    settings.SetDiskCacheDir(ANTIROBOT_DAEMON_CONFIG.TVMClientCacheDir);
    settings.EnableServiceTicketChecking();
    settings.EnableServiceTicketsFetchOptions(tvmSecret, NTvmAuth::NTvmApi::TClientSettings::TDstVector{
        ANTIROBOT_DAEMON_CONFIG.CbbTVMClientId,
        ANTIROBOT_DAEMON_CONFIG.FuryTVMClientId,
        ANTIROBOT_DAEMON_CONFIG.FuryPreprodTVMClientId,
    });

    const size_t maxTry = 5;
    for (size_t attempt = 1; ; ++attempt) {
        try {
            const int warnLogLevel = 4;
            TvmClient.Reset(new NTvmAuth::TTvmClient(settings, new NTvmAuth::TCerrLogger(warnLogLevel)));
            break;
        } catch (NTvmAuth::TRetriableException& ex) {
            Cerr << ex.what() << Endl;
            if (attempt > maxTry) {
                ythrow yexception() << "Fatal: could not initialize TTvmClient";
            }
        }
    }
}

TString TAntirobotTvm::GetServiceTicket(NTvmAuth::TTvmId targetClientId) const {
    if (!TvmClient) {
        return TString();
    }

    try {
        return TvmClient->GetServiceTicketFor(targetClientId);
    } catch (...) {
        EVLOG_MSG << "Could not get service ticket " << targetClientId << ": " << CurrentExceptionMessage();
        Cerr << "Could not get service ticket " << targetClientId << ": " << CurrentExceptionMessage() << Endl;
    }
    return TString();
}

TAntirobotTvmClientInfo TAntirobotTvm::CheckClientTicket(TStringBuf ticket) const {
    if (ANTIROBOT_DAEMON_CONFIG.Local && ticket == "test") {
        return {1, "test"};
    }
    if (!TvmClient) {
        return {0, "unknown"};
    }

    const NTvmAuth::TCheckedServiceTicket serviceTicket = TvmClient->CheckServiceTicket(ticket);
    const NTvmAuth::ETicketStatus status = serviceTicket.GetStatus();
    if (status != NTvmAuth::ETicketStatus::Ok) {
        ythrow TInvalidTvmClientException() << "Incorrect service ticket status: " << ToString(static_cast<int>(status));
    }
    NTvmAuth::TTvmId clientId = serviceTicket.GetSrc();
    const auto ptr = AllowedClients.FindPtr(clientId);
    if (ptr == nullptr) {
        ythrow TInvalidTvmClientException() << "Client id " << ToString(clientId) << " is now allowed";
    }
    return *ptr;
}
