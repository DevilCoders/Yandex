#pragma once

#include <kernel/common_server/ciphers/pool/abstract/abstract.h>
#include <kernel/common_server/ciphers/aes.h>
#include <kernel/common_server/ciphers/pool/db/dek.h>
#include <kernel/common_server/api/history/cache.h>
#include <kernel/common_server/common/manager_config.h>

namespace NCS {

    class TDBPoolKeyCipherConfig: public NCommon::TManagerConfig, public ICipherConfig {
    private:
        using TBase = ICipherConfig;
        CSA_DEFAULT(TDBPoolKeyCipherConfig, TString, DBName);
        CSA_DEFAULT(TDBPoolKeyCipherConfig, TString, KekCipherName);
        CS_ACCESS(TDBPoolKeyCipherConfig, TString, CipherMethod, "aes-256-gcm");
        CS_ACCESS(TDBPoolKeyCipherConfig, ui32, PoolSize, 10);
        CS_ACCESS(TDBPoolKeyCipherConfig, ui32, DekTTL, 1000);

        static TFactory::TRegistrator<TDBPoolKeyCipherConfig> Registrator;

    public:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* server) const override;
        void DoInit(const TYandexConfig::Section* section) override;
        void DoToString(IOutputStream& os) const override;

        virtual TString GeneratePlainDek() const;
        virtual IKeyEncryptedCipher::TPtr ConstructDekCipher(const IKeyCipher::TCipherKey& key, const IAbstractCipher::TPtr KekCipher) const;

        using TBase::TBase;
    };

    class TDBPoolKeyCipher: public TDBEntitiesCache<TDBDek>, public IPoolCipher {
    private:
        using TBase = TDBEntitiesCache<TDBDek>;

        using TMapPool = TMap<TString, TDBDek::TPtr>;
        CSA_MUTABLE_DEF(TDBPoolKeyCipher, TMapPool, DekPool);
        CSA_MUTABLE_DEF(TDBPoolKeyCipher, TMapPool, OnlyDecryption);
        CSA_MUTABLE_DEF(TDBPoolKeyCipher, TVector<TDBDek::TPtr>, DekArray);

        const IAbstractCipher::TPtr KekCipher;
        const IKeyEncryptedCipher::TPtr ConstructDekCipher(const TDBDek::TPtr encryptedDek) const;
        const TDBPoolKeyCipherConfig Config;

        bool UpsertDeks(const TVector<TDBDek>& objects, NCS::TEntitySession& session) const;
        TDBDek::TPtr GenerateDek() const;
        bool RegenerateDeks() const;
        void RebuildDekArray() const;
        bool ActualizeDeks() const;

        mutable TString KEKVersion;
        TThreadPool KEKVersionRefreshPool;
        mutable TRWMutex KEKVersionMutex;
        class TKEKVersionRefreshAgent: public IObjectInQueue {
        private:
            const TDBPoolKeyCipher* Owner;

        public:
            TKEKVersionRefreshAgent(const TDBPoolKeyCipher* owner)
                : Owner(owner)
            {
            }

            virtual void Process(void* /*threadSpecificResource*/) override;
        };

        TString GetKEKVersion() const {
            TReadGuard wg(KEKVersionMutex);
            return KEKVersion;
        }
        void SetKEKVersion(const TString& version) const {
            TWriteGuard wg(KEKVersionMutex);
            KEKVersion = version;
        }

    protected:
        virtual bool DoStart() override;
        virtual bool DoStop() override;
        virtual IAbstractCipher::TPtr GetCipherForEncryption() const override;
        virtual IAbstractCipher::TPtr GetCipherForDecryption(const TString& encrypted) const override;

        virtual IAbstractCipher::TPtr DoCreateNewVersion() const override {
            auto newCipher = KekCipher->CreateNewVersion();
            return MakeAtomicShared<TDBPoolKeyCipher>(TBase::GetDatabase(), Config, !!newCipher ? newCipher : KekCipher);
        }

    public:
        bool ActualizeEncryptedData() const;

        TDBPoolKeyCipher(NStorage::IDatabase::TPtr db, const TDBPoolKeyCipherConfig& config, const IAbstractCipher::TPtr kekCipher)
            : TBase(db, config.GetHistoryConfig())
            , KekCipher(kekCipher)
            , Config(config)
        {
            CHECK_WITH_LOG(!!KekCipher);
        }
    };
}
