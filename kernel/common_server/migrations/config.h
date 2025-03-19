#pragma once

#include "migration.h"
#include <kernel/common_server/api/history/db_entities.h>

namespace NCS {
    namespace NStorage {

        class IDBMigrationsSource {
        public:
            using TPtr = TAtomicSharedPtr<IDBMigrationsSource>;
            virtual ~IDBMigrationsSource() = default;
            virtual TVector<TDBMigration> CreateMigrations() const = 0;
            virtual TString GetDBName() const = 0;
        };

        class TDBMigrationsSourceConfig {
        public:
            using TPtr = TAtomicSharedPtr<TDBMigrationsSourceConfig>;
            using TFactory = NObjectFactory::TObjectFactory<TDBMigrationsSourceConfig, TString>;

            virtual ~TDBMigrationsSourceConfig() = default;
            virtual IDBMigrationsSource::TPtr ConstructSource() const = 0;
            virtual TString GetClassName() const = 0;
            void Init(const TYandexConfig::Section* section, const TString& dbName);
            void ToString(IOutputStream& os) const;

            CSA_READONLY_DEF(TString, Name);
            CSA_READONLY_DEF(TString, DBName);
            CSA_READONLY_DEF(TMigrationHeader, DefaultHeader);

        protected:
            virtual void DoInit(const TYandexConfig::Section* section) = 0;
            virtual void DoToString(IOutputStream& os) const = 0;
        };

        class TDBMigrationsSource: public IDBMigrationsSource {
        public:
            TDBMigrationsSource(const TDBMigrationsSourceConfig& config);
            virtual TVector<TDBMigration> CreateMigrations() const final;
            virtual TString GetDBName() const override {
                return Config.GetDBName();
            }

        protected:
            virtual TVector<TDBMigration> DoCreateMigrations() const = 0;

        private:
            const TDBMigrationsSourceConfig& Config;
        };

        using TDBMigrationSourcesConfig = TBaseInterfaceContainer<TDBMigrationsSourceConfig>;

        class TDBMigrationsConfig: public TDBEntitiesManagerConfig {
            CSA_READONLY_DEF(TVector<TDBMigrationSourcesConfig>, MigrationSources);

        public:
            void Init(const TYandexConfig::Section* section, const TString& dbName);
            void ToString(IOutputStream& os) const;
        };

    }
}

