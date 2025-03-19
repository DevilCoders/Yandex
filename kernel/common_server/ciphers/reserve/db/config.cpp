#include "config.h"

#include <kernel/common_server/ciphers/reserve/db/cipher.h>

namespace NCS {
    TDBReservedKeyCipherConfig::TFactory::TRegistrator<TDBReservedKeyCipherConfig> TDBReservedKeyCipherConfig::Registrator("reserve");

    IAbstractCipher::TPtr TDBReservedKeyCipherConfig::DoConstruct(const IBaseServer* server) const {
        if (!server) {
            return nullptr;
        }
        auto cipher = MakeAtomicShared<TDBReservedKeyCipher>(server->GetDatabase(DBName), *this, server->GetCipherPtr(MainCipherName), server->GetCipherPtr(ReserveCipherName));
        cipher->Start();
        return cipher;
    }

    void TDBReservedKeyCipherConfig::DoInit(const TYandexConfig::Section* section) {
        NCommon::TManagerConfig::Init(section);
        DBName = section->GetDirectives().Value<TString>("DBName", DBName);
        MainCipherName = section->GetDirectives().Value<TString>("MainCipherName", MainCipherName);
        ReserveCipherName = section->GetDirectives().Value<TString>("ReserveCipherName", ReserveCipherName);
    }

    void TDBReservedKeyCipherConfig::DoToString(IOutputStream& os) const {
        NCommon::TManagerConfig::ToString(os);
        os << "DBName: " << DBName << Endl;
        os << "MainCipherName: " << MainCipherName << Endl;
        os << "ReserveCipherName: " << ReserveCipherName << Endl;
    }

}