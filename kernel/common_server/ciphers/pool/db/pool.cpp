#include "pool.h"

#include <kernel/common_server/library/openssl/aes.h>

namespace NCS {
    TDBPoolKeyCipherConfig::TFactory::TRegistrator<TDBPoolKeyCipherConfig> TDBPoolKeyCipherConfig::Registrator("pool");

    void TDBPoolKeyCipher::TKEKVersionRefreshAgent::Process(void* /*threadSpecificResource*/) {
        while (true) {
            Owner->ActualizeEncryptedData();
            Sleep(TDuration::Minutes(5));
        }
    }

    IAbstractCipher::TPtr TDBPoolKeyCipherConfig::DoConstruct(const IBaseServer* server) const {
        if (!server) {
            return nullptr;
        }
        auto cipher = MakeAtomicShared<TDBPoolKeyCipher>(server->GetDatabase(DBName), *this, server->GetCipherPtr(KekCipherName));
        cipher->Start();
        return cipher;
    }

    void TDBPoolKeyCipherConfig::DoInit(const TYandexConfig::Section* section) {
        NCommon::TManagerConfig::Init(section);
        DBName = section->GetDirectives().Value<TString>("DBName", DBName);
        KekCipherName = section->GetDirectives().Value<TString>("KekCipherName", KekCipherName);
        CipherMethod = section->GetDirectives().Value<TString>("CipherMethod", CipherMethod);
        PoolSize = section->GetDirectives().Value<ui32>("PoolSize", PoolSize);
        DekTTL = section->GetDirectives().Value<ui32>("DekTTL", DekTTL);
    }

    void TDBPoolKeyCipherConfig::DoToString(IOutputStream& os) const {
        NCommon::TManagerConfig::ToString(os);
        os << "DBName: " << DBName << Endl;
        os << "KekCipherName: " << KekCipherName << Endl;
        os << "CipherMethod: " << CipherMethod << Endl;
        os << "PoolSize: " << ::ToString(PoolSize) << Endl;
        os << "DekTTL: " << ::ToString(DekTTL) << Endl;
    }

    TString TDBPoolKeyCipherConfig::GeneratePlainDek() const {
        return NOpenssl::GenerateAESKey(32);
    }

    IAbstractCipher::TPtr TDBPoolKeyCipher::GetCipherForEncryption() const {
        if (DekArray.empty()) {
            return nullptr;
        }
        const auto dek = DekArray[RandomNumber<size_t>(DekArray.size())];
        if (!dek) {
            return nullptr;
        }
        --dek->MutableTTL();
        {
            auto session = BuildNativeSession(false);
            if (!UpsertObject(*dek, "dek_pool", session) || !session.Commit()) {
                TFLEventLog::Error("cannot decrease dek TTL");
            }
        }
        if (dek->GetTTL() == 0) {
            auto node = DekPool.extract(dek->GetId());
            OnlyDecryption.insert(std::move(node));
            if (!RegenerateDeks()) {
                TFLEventLog::Error("cannot add new dek");
            }
        }
        return ConstructDekCipher(dek);
    }

    IAbstractCipher::TPtr TDBPoolKeyCipher::GetCipherForDecryption(const TString& encrypted) const {
        auto it = DekPool.find(TAesGcmKeyEncryptedCipher::GetKeyId(encrypted));
        if (it != DekPool.end()) {
            return ConstructDekCipher(it->second);
        }

        auto it2 = OnlyDecryption.find(TAesGcmKeyEncryptedCipher::GetKeyId(encrypted));
        if (it2 != OnlyDecryption.end()) {
            return ConstructDekCipher(it2->second);
        }

        return nullptr;
    }

    bool TDBPoolKeyCipher::DoStart() {
        if (!TBase::DoStart()) {
            return false;
        }
        if (!ActualizeDeks()) {
            return false;
        }

        KEKVersionRefreshPool.Start(1);
        KEKVersionRefreshPool.SafeAddAndOwn(MakeHolder<TKEKVersionRefreshAgent>(this));

        return true;
    }

    bool TDBPoolKeyCipher::DoStop() {
        if (!TBase::DoStop()) {
            return false;
        }
        KEKVersionRefreshPool.Stop();
        return true;
    }

    void TDBPoolKeyCipher::RebuildDekArray() const {
        DekArray.clear();
        for (const auto& i : DekPool) {
            DekArray.emplace_back(i.second);
        }
    }

    const IKeyEncryptedCipher::TPtr TDBPoolKeyCipher::ConstructDekCipher(const TDBDek::TPtr encryptedDek) const {
        return Config.ConstructDekCipher(IKeyCipher::TCipherKey(encryptedDek->GetEncrypted(), encryptedDek->GetId()), KekCipher);
    }

    bool TDBPoolKeyCipher::RegenerateDeks() const {
        if (DekPool.size() >= Config.GetPoolSize()) {
            RebuildDekArray();
            return true;
        }

        auto session = BuildNativeSession(false);
        TVector<TDBDek> upsertDeks;
        for (size_t i = DekPool.size(); i < Config.GetPoolSize(); ++i) {
            const auto dek = GenerateDek();
            if (!dek) {
                return false;
            }
            DekPool.emplace(dek->GetId(), dek);
            upsertDeks.push_back(*dek);
        }
        if (!UpsertDeks(upsertDeks, session) || !session.Commit()) {
            return false;
        }

        RebuildDekArray();
        return true;
    }

    TDBDek::TPtr TDBPoolKeyCipher::GenerateDek() const {
        const TString plainDek = Config.GeneratePlainDek();
        TString encrypted;
        if (!KekCipher->Encrypt(plainDek, encrypted)) {
            return nullptr;
        }

        auto dek = MakeAtomicShared<TDBDek>();
        dek->SetEncrypted(encrypted);
        dek->SetTTL(Config.GetDekTTL());
        return dek;
    }

    bool TDBPoolKeyCipher::UpsertDeks(const TVector<TDBDek>& objects, NCS::TEntitySession& session) const {
        return AddObjects(objects, "deks_pool", session);
    }

    IKeyEncryptedCipher::TPtr TDBPoolKeyCipherConfig::ConstructDekCipher(const IKeyCipher::TCipherKey& key, const IAbstractCipher::TPtr KekCipher) const {
        if (CipherMethod == "aes-256-gcm") {
            auto dekCipher = MakeAtomicShared<TAESGcmCipher>(IKeyCipher::TCipherKey());
            return MakeAtomicShared<TAesGcmKeyEncryptedCipher>(std::move(dekCipher), KekCipher, key);
        }

        return nullptr;
    }

    bool TDBPoolKeyCipher::ActualizeDeks() const {
        TVector<TDBDek> deks;
        {
            auto session = BuildNativeSession(true);
            if (!RestoreAllObjects(deks, session)) {
                return false;
            }
        }

        DekPool.clear();
        OnlyDecryption.clear();

        for (auto&& i : deks) {
            if (i.GetTTL() > 0) {
                DekPool.emplace(i.GetId(), MakeAtomicShared<TDBDek>(i));
            } else {
                OnlyDecryption.emplace(i.GetId(), MakeAtomicShared<TDBDek>(i));
            }
        }

        if (!RegenerateDeks()) {
            return false;
        }
        return true;
    }

    bool TDBPoolKeyCipher::ActualizeEncryptedData() const {
        const TString kekVersion = KekCipher->GetVersion();
        if (GetKEKVersion() == kekVersion) {
            return true;
        }
        TVector<TDBDek> deks;
        {
            auto session = BuildNativeSession(true);
            if (!RestoreAllObjects(deks, session)) {
                return false;
            }
        }

        TVector<TDBDek> deksForUpdate;
        for (auto&& i : deks) {
            bool isReencrypted = false;
            if (!KekCipher->ReencryptIfNeeded(i.MutableEncrypted(), isReencrypted)) {
                return false;
            }
            if (isReencrypted) {
                deksForUpdate.emplace_back(std::move(i));
            }
        }

        if (deksForUpdate.empty()) {
            SetKEKVersion(kekVersion);
            return true;
        }

        auto session = BuildNativeSession(false);
        for (auto&& i : deksForUpdate) {
            if (!UpsertObject(i, "dek_pool", session)) {
                return false;
            }
        }

        if (!session.Commit()) {
            return false;
        }
        if (!ActualizeDeks()) {
            return false;
        }

        SetKEKVersion(kekVersion);

        return true;
    }
}
