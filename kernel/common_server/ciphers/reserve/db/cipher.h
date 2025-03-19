#pragma once

#include "config.h"
#include "object.h"

#include <kernel/common_server/ciphers/reserve/abstract.h>
#include <kernel/common_server/api/history/cache.h>
#include <kernel/common_server/common/manager_config.h>

namespace NCS {
    class TDBReservedKeyCipher: public TDBEntitiesCache<TDBReserveEncrypted>, public ICipherWithReserve {
    private:
        using TBase = TDBEntitiesCache<TDBReserveEncrypted>;

        class TIndexByHashPolicy {
        public:
            using TKey = TString;
            using TObject = TDBReserveEncrypted;
            static const TString& GetKey(const TDBReserveEncrypted& object) {
                return object.GetHash();
            }
            static const ui64& GetUniqueId(const TDBReserveEncrypted& object) {
                return object.GetReserveId();
            }
        };

        class TIndexByReserveHashPolicy {
        public:
            using TKey = TString;
            using TObject = TDBReserveEncrypted;
            static const TString& GetKey(const TDBReserveEncrypted& object) {
                return object.GetReserveHash();
            }
            static const ui64& GetUniqueId(const TDBReserveEncrypted& object) {
                return object.GetReserveId();
            }
        };

        using TIndexByHash = TObjectByKeyIndex<TIndexByHashPolicy>;
        using TIndexByReserveHash = TObjectByKeyIndex<TIndexByReserveHashPolicy>;

        mutable TIndexByHash IndexByHash;
        mutable TIndexByReserveHash IndexByReserveHash;

        const TDBReservedKeyCipherConfig Config;

    protected:
        virtual bool Store(const TString& encrypted, const TString& reserveEncrypted = "") const override;
        virtual bool Restore(const TString& encrypted, TString& reserveEncrypted) const override;

        virtual bool DoRebuildCacheUnsafe() const override;
        virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBReserveEncrypted>& ev) const override;
        virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBReserveEncrypted>& ev, TDBReserveEncrypted& object) const override;

        virtual IAbstractCipher::TPtr DoCreateNewVersion() const override {
            return MakeAtomicShared<TDBReservedKeyCipher>(TBase::GetDatabase(), Config, MainCipher->CreateNewVersion(), ReserveCipher->CreateNewVersion());
        }

        TVector<TDBReserveEncrypted> GetByReserveHash(const TString& hash) const;
        TVector<TDBReserveEncrypted> GetByHash(const TString& hash) const;

    public:
        TDBReservedKeyCipher(NStorage::IDatabase::TPtr db, const TDBReservedKeyCipherConfig& config, const IAbstractCipher::TPtr main, const IAbstractCipher::TPtr reserve)
            : TBase(db, config.GetHistoryConfig())
            , ICipherWithReserve(main, reserve)
            , Config(config)
        {
        }
    };
}