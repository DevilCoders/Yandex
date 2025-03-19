#pragma once
#include <kernel/common_server/library/kv/abstract/storage.h>
#include <kernel/common_server/library/storage/abstract/database.h>
#include <util/generic/hash_set.h>
#include <util/system/mutex.h>

namespace NCS {
    namespace NKVStorage {
        class TDBStorage: public IStorage {
        private:
            using TBase = IStorage;
            const NCS::NStorage::IDatabase::TPtr Database;
            const TString DefaultTableName;

            TRWMutex MutexExistedTables;
            using TExistedTables = THashSet<TString>;
            mutable TAtomicSharedPtr<TExistedTables> ExistedTables;

        protected:
            virtual bool DoPutData(const TString& path, const TBlob& data, const TWriteOptions& options) const override;
            virtual bool DoGetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options) const override;
            virtual TString DoGetPathByHash(const TBlob& data, const TReadOptions& options) const override;
            virtual bool DoPrepareEnvironment(const TBaseOptions& options) const override;

        public:
            TDBStorage(const TString& storageName, const ui32 threadsCount, NCS::NStorage::IDatabase::TPtr db, const TString& defaultTableName)
                : TBase(storageName, threadsCount)
                , Database(db)
                , DefaultTableName(defaultTableName)
                , ExistedTables(MakeAtomicShared<TExistedTables>())
            {
                CHECK_WITH_LOG(db);
            }
        };
    }
}
