#pragma once
#include "config.h"
#include "object.h"
#include <kernel/common_server/api/history/db_entities.h>

namespace NCS {
    namespace NResources {
        class TDBManager {
        private:
            NStorage::IDatabase::TPtr Database;
            const TDBManagerConfig Config;
        public:
            TDBManager(NStorage::IDatabase::TPtr db, const TDBManagerConfig& config)
                : Database(db)
                , Config(config)
            {

            }
            NCS::TEntitySession BuildNativeSession(const bool readOnly) const;
            bool RestoreByAccessId(const TString& accessId, TVector<TDBResource>& result, NCS::TEntitySession& session) const;
            bool RestoreKeysByAccessId(const TString& accessId, TSet<TString>& result, NCS::TEntitySession& session) const;
            bool RestoreByResourceKey(const TString& resourceKey, TMaybe<TDBResource>& result, NCS::TEntitySession& session) const;
            bool RemoveObjects(const TSet<TString>& keys, NCS::TEntitySession& session) const;
            bool UpsertObjects(const TVector<TDBResource>& objects, NCS::TEntitySession& session) const;
            bool RemoveObject(const TString& key, NCS::TEntitySession& session) const {
                return RemoveObjects({ key }, session);
            }
            bool UpsertObject(const TDBResource& object, NCS::TEntitySession& session) const {
                return UpsertObjects({ object }, session);
            }
        };
    }
}

