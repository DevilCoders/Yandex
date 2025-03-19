#pragma once
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/tvmauth/checked_user_ticket.h>
#include <library/cpp/tvmauth/client/misc/api/settings.h>
#include <library/cpp/yconf/conf.h>
#include <util/generic/set.h>

class TTvmConfig {
public:
    using TClientIdsAndAliases = TMap<ui32, TVector<TString>>;
    
private:
    CSA_READONLY_DEF(TString, Cache);
    CSA_READONLY_DEF(TString, Secret);
    CSA_READONLY_DEF(TString, SecretFile);
    CSA_READONLY_DEF(TClientIdsAndAliases, DestinationClientIds);
    CSA_READONLY(ui32, SelfClientId, 0);
    CSA_READONLY_FLAG(DefaultClient, false);
    CSA_READONLY_DEF(NTvmAuth::EBlackboxEnv, BlackboxEnv);
    CSA_READONLY_DEF(TString, Host);
    CSA_READONLY(ui16, Port, 0);
    CSA_READONLY_DEF(TString, Name);

public:
    NTvmAuth::NTvmApi::TClientSettings::TDstMap BuildDestinationsMap() const;

    void Init(const TYandexConfig::Section* section);
    void ToString(IOutputStream& os) const;
    
};

