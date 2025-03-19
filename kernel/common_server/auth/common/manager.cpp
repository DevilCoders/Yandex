#include "manager.h"

#include <library/cpp/logger/global/global.h>

namespace {
    class TTvmLogger: public NTvmAuth::ILogger {
        void Log(int lvl, const TString& msg) override {
            TEMPLATE_LOG(static_cast<ELogPriority>(lvl)) << msg << Endl;
        }
    };
}

NCS::TTvmManager::TTvmManager(const TMap<TString, TTvmConfig>& configs)
        : TvmConfigs(configs)
    {
    for (auto&& [name, config]: TvmConfigs) {
        NTvmAuth::NTvmApi::TClientSettings settings;
        settings.SetSelfTvmId(config.GetSelfClientId());
        settings.EnableServiceTicketChecking();
        settings.EnableUserTicketChecking(config.GetBlackboxEnv());
        if (!!config.GetHost()) {
            settings.TvmHost = config.GetHost();
        }
        if (!!config.GetPort()) {
            settings.TvmPort = config.GetPort();
        }
        if (const auto& cache = config.GetCache()) {
            settings.SetDiskCacheDir(cache);
        }
        if (const auto& secret = config.GetSecret()) {
            settings.EnableServiceTicketsFetchOptions(secret, config.BuildDestinationsMap());
        }
        TvmClients[name] = MakeAtomicShared<NTvmAuth::TTvmClient>(settings, MakeIntrusive<TTvmLogger>());
        if (config.IsDefaultClient()) {
            DefaultClientId = name;
        }
    }
}

TAtomicSharedPtr<NTvmAuth::TTvmClient> NCS::TTvmManager::GetTvmClient(const TString& name) const {
    auto p = TvmClients.find(name);
    if (p != TvmClients.end()) {
        return p->second;
    } else {
        return nullptr;
    }
}
