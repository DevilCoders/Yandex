#include "tvm.h"

#include <antirobot/cbb/cbb_fast/protos/config.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json_value.h>

#include <util/stream/file.h>
#include <util/system/env.h>
#include <util/folder/dirut.h>

namespace {
    const TString TVM_SECRET_ENV = "TVM_SECRET";
} // anonymous namespace

namespace NCbbFast {

TTvm::TTvm(const TConfig& config) {
    if (config.GetTvmClientsList()) {
        TFileInput confFile(config.GetTvmClientsList());
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

    MakeDirIfNotExist(config.GetTvmClientCacheDir());

    NTvmAuth::NTvmApi::TClientSettings settings;

    settings.SetSelfTvmId(config.GetTvmClientId());
    settings.SetDiskCacheDir(config.GetTvmClientCacheDir());
    settings.EnableServiceTicketChecking();

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

TTvmClientInfo TTvm::CheckClientTicket(TStringBuf ticket) const {
    if (!TvmClient) {
        return {0, "unknown"};
    }

    const NTvmAuth::TCheckedServiceTicket serviceTicket = TvmClient->CheckServiceTicket(ticket);
    const NTvmAuth::ETicketStatus status = serviceTicket.GetStatus();
    if (status != NTvmAuth::ETicketStatus::Ok) {
        ythrow TInvalidTvmClientException() << "Incorrect service ticket status: " << ToString(static_cast<int>(status));
    }

    NTvmAuth::TTvmId clientId = serviceTicket.GetSrc();
    const auto *const ptr = AllowedClients.FindPtr(clientId);
    if (ptr == nullptr) {
        ythrow TInvalidTvmClientException() << "Client id " << ToString(clientId) << " is now allowed";
    }
    return *ptr;
}

} // namespace NCbbFast
