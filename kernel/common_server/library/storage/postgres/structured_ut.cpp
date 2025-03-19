#include "table_accessor.h"

#include <kernel/common_server/migrations/manager.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/builder.h>
#include <util/system/env.h>


namespace {
    TString GetConnectionString() {
        TStringBuilder connectionString;
        connectionString << "host=" << GetEnv("POSTGRES_RECIPE_HOST");
        connectionString << " port=" << GetEnv("POSTGRES_RECIPE_PORT");
        connectionString << " target_session_attrs=read-write dbname=" << GetEnv("POSTGRES_RECIPE_DBNAME");
        connectionString << " user=" << GetEnv("POSTGRES_RECIPE_USER");
        connectionString << " password=" << Endl;
        return connectionString;
    }

    TDatabasePtr InitDb(const NCS::NStorage::TPostgresStorageOptionsImpl* cfg = nullptr) {
        NCS::NStorage::TPostgresStorageOptionsImpl config;
        if (cfg) {
            config = *cfg;
        }
        config.SetSimpleConnectionString(GetConnectionString());
        TDatabasePtr db = MakeAtomicShared<NCS::NStorage::TPostgresDB>(config, "test", GetEnv("POSTGRES_RECIPE_DBNAME"));
        TStringBuilder sb;
        const auto folders = StringSplitter(GetEnv("PG_MIGRATIONS_DIR")).SplitBySet(", ").SkipEmpty().ToList<TString>();
        sb << "<Sources>" << Endl;
        ui32 idx = 0;
        for (const auto& folder : folders) {
            sb << "<m" << ++idx << ">" << Endl;
            sb << "Type: folder" << Endl;
            sb << "MaxAttemptsPerMigration: 1" << Endl;
            sb << "Path: " << folder << Endl;
            sb << "</m" << idx << ">" << Endl;
        }
        sb << "</Sources>" << Endl;
        TAnyYandexConfig yandCfg;
        UNIT_ASSERT(yandCfg.ParseMemory(sb));
        NCS::NStorage::TDBMigrationsConfig migrationsCfg;
        migrationsCfg.Init(yandCfg.GetRootSection(), "test");
        NCS::NStorage::TDBMigrationsManager mm(THistoryContext(db), migrationsCfg, nullptr);
        CHECK_WITH_LOG(mm.ApplyMigrations());
        return db;
    }

}
Y_UNIT_TEST_SUITE(PostgresTestSuite) {
    Y_UNIT_TEST(MultiExec) {
        auto db = InitDb();
        {
            auto tx = db->CreateTransaction();
            UNIT_ASSERT(tx);

            TRecordsSetWT first;
            TRecordsSetWT third;
            auto result = tx->MultiExec({
                { "SELECT 1 as first", first },
                { "SELECT 2 as second" },
                { "SELECT 3 as third", third },
                { "ROLLBACK" }
                });
            UNIT_ASSERT(!!result);
            UNIT_ASSERT_VALUES_EQUAL(result.GetAffectedRows(), 3);
            UNIT_ASSERT_VALUES_EQUAL(first.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(third.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(*first.front().CastTo<ui32>("first"), 1);
            UNIT_ASSERT_VALUES_EQUAL(*third.front().CastTo<ui32>("third"), 3);
        }
    }

    Y_UNIT_TEST(MultiExecRollback) {
        NCS::NStorage::TPostgresStorageOptionsImpl config;
        config.SetHardLimitPoolSize(2);
        auto db = InitDb(&config);
        auto tx0 = db->CreateTransaction();
        UNIT_ASSERT(tx0);

        auto lock = tx0->Exec("LOCK TABLE txn_test IN EXCLUSIVE MODE");
        UNIT_ASSERT(lock);
        UNIT_ASSERT(lock->IsSucceed());
        {
            auto tx = db->CreateTransaction();
            UNIT_ASSERT(tx);

            auto result = tx->MultiExec({
                { "LOCK TABLE txn_test IN SHARE MODE NOWAIT" },
                { "SELECT 1" },
                { "ROLLBACK" }
            });
            UNIT_ASSERT(!result);
        }
        {
            auto tx = db->CreateTransaction();
            UNIT_ASSERT(tx);

            auto result = tx->Exec("SELECT 1");
            UNIT_ASSERT(!!result);
        }
    }

    Y_UNIT_TEST(Savepoint) {
        auto db = InitDb();
        auto tx0 = db->CreateTransaction();
        UNIT_ASSERT(tx0);

        auto savepoint = "savepoint_" + ToString(MicroSeconds());
        auto result = tx0->MultiExec({
            { "SAVEPOINT " + savepoint },
            { "LOCK TABLE txn_test IN ACCESS EXCLUSIVE MODE" },
            { "SELECT * FROM txn_test" },
            { "ROLLBACK TO SAVEPOINT " + savepoint }
        });
        UNIT_ASSERT(!!result);

        {
            auto tx = db->CreateTransaction();
            UNIT_ASSERT(tx);

            auto result = tx->Exec("SELECT * FROM txn_test");
            UNIT_ASSERT(!!result);
        }
    }
}
