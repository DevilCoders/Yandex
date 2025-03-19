#pragma once

#include <library/cpp/tvmauth/client/facade.h>
#include <kernel/common_server/library/interfaces/tvm_manager.h>
#include <kernel/common_server/auth/common/tvm_config.h>
#include <kernel/common_server/util/auto_actualization.h>

namespace NCS {
    class TTvmManager: public ITvmManager {
    public:
        TTvmManager(const TMap<TString, TTvmConfig>& configs);
        virtual ~TTvmManager() = default;

        TAtomicSharedPtr<NTvmAuth::TTvmClient> GetTvmClient(const TString& name) const override;

    private:
        TMap<TString, TAtomicSharedPtr<NTvmAuth::TTvmClient>> TvmClients;
        const TMap<TString, TTvmConfig>& TvmConfigs;
        CSA_READONLY(TString, DefaultClientId, 0);
    };
}
