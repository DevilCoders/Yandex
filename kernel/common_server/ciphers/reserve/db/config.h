#pragma once

#include <kernel/common_server/ciphers/reserve/abstract.h>
#include <kernel/common_server/ciphers/config.h>
#include <kernel/common_server/common/manager_config.h>

namespace NCS {

    class TDBReservedKeyCipherConfig: public NCommon::TManagerConfig, public ICipherConfig {
    private:
        using TBase = ICipherConfig;
        CSA_DEFAULT(TDBReservedKeyCipherConfig, TString, DBName);
        CSA_DEFAULT(TDBReservedKeyCipherConfig, TString, MainCipherName);
        CSA_DEFAULT(TDBReservedKeyCipherConfig, TString, ReserveCipherName);

        static TFactory::TRegistrator<TDBReservedKeyCipherConfig> Registrator;

    public:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* server) const override;
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;

        using TBase::TBase;
    };
}