#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/library/storage/ydb/structured.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/random.h>
#include <util/system/env.h>

using namespace NYdb;
using namespace NYdb::NTable;

void FillTable(NStorage::IDatabase& db, const TString& table) {
    if (!db.DropTable(table)) {
        INFO_LOG << "cannot drop table: " << table << Endl;
    }

    const TString constrScript = R"({
        "columns": {
            "snapshot_id": "Uint32",
            "object_id": "Uint64",
            "data": "String"
        },
        "primary_key": ["object_id"]
    })";
    UNIT_ASSERT(db.CreateTable(table, constrScript));

    auto query = Sprintf(R"(
        REPLACE INTO %s
        (
            snapshot_id,
            object_id,
            data
        )
        VALUES
        (
            1,
            1,
            "data 1"
        ),
        (
            1,
            2,
            "data 2"
        ),
        (
            1,
            3,
            "data 3"
        ),
        (
            1,
            4,
            "data 4"
        );
    )",
        table.c_str()
    );
    auto tx = db.CreateTransaction(false);
    UNIT_ASSERT(tx);

    auto replaceResult = tx->Exec(query);
    UNIT_ASSERT(!!replaceResult);
    UNIT_ASSERT(tx->Commit());
}

NStorage::IDatabase::TPtr CreateAndFillDb(const TString& table) {
    NCS::NStorage::TYDBDatabaseConfig config;
    config
        .SetEndpoint(GetEnv("YDB_ENDPOINT"))
        .SetDBName(GetEnv("YDB_DATABASE"));
    if (TString token = GetEnv("YDB_TOKEN")) {
        auto authConfig = MakeHolder<NCS::TAuthConfigOAuth>();
        authConfig->SetToken(token);
        config.SetAuthConfig(std::move(authConfig));
    }

    config.SetTablePath("/" + config.GetDBName());
    auto db = config.ConstructDatabase(nullptr);
    FillTable(*db, table);
    return db;

}

Y_UNIT_TEST_SUITE(YDBTestSuite) {
    Y_UNIT_TEST(SELECTTransactionExec1) {
        constexpr const char* table = "SELECTTransactionExec1";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);
            UNIT_ASSERT(tx);

            auto selectResult = tx->Exec(Sprintf(R"(SELECT snapshot_id, object_id, data FROM %s;)", table));
            UNIT_ASSERT(!!selectResult);
            UNIT_ASSERT_VALUES_EQUAL(selectResult.GetAffectedRows(), 4);

            UNIT_ASSERT(tx->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(SELECTTransactionExec1)

    Y_UNIT_TEST(DELETETransactionExec1) {
        constexpr const char* table = "DELETETransactionExec1";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);
            UNIT_ASSERT(tx);

            auto deleteResult = tx->Exec(Sprintf(R"(DELETE FROM %s WHERE snapshot_id == 1 AND object_id == 3;)", table));
            UNIT_ASSERT(!!deleteResult);

            UNIT_ASSERT(tx->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(DELETETransactionExec1)

    Y_UNIT_TEST(INSERTTransactionExec) {
        constexpr const char* table = "INSERTTransactionExec";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);
            UNIT_ASSERT(tx);

            auto insertResult = tx->Exec(Sprintf(R"(
                INSERT INTO %s (snapshot_id , object_id, data)
                VALUES (1, 5, "data 5");)", table));
            UNIT_ASSERT(!!insertResult);

            UNIT_ASSERT(tx->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(INSERTTransactionExec)

    Y_UNIT_TEST(REPLACETransactionExec) {
        constexpr const char* table = "REPLACETransactionExec";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);
            UNIT_ASSERT(tx);

            auto replaceResult = tx->Exec(Sprintf(R"(REPLACE INTO %s (snapshot_id , object_id, data) VALUES (1, 5, "data 5-2");)", table));
            UNIT_ASSERT(!!replaceResult);

            UNIT_ASSERT(tx->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(REPLACETransactionExec)

    Y_UNIT_TEST(UPSERTTransactionExec) {
        constexpr const char* table = "UPSERTTransactionExec";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);
            UNIT_ASSERT(tx);

            auto updateResult = tx->Exec(Sprintf(R"(UPSERT INTO %s (snapshot_id , object_id, data) VALUES (1, 5, "data 5-3");)", table));
            UNIT_ASSERT(!!updateResult);

            UNIT_ASSERT(tx->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(UPSERTTransactionExec)

    Y_UNIT_TEST(TableAccessorGetRows) {
        constexpr const char* table = "TableAccessorGetRows";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(true);
            UNIT_ASSERT(tx);

            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);

            TRecordsSetWT readingRecords;
            TSRSelect srSelect(table, &readingRecords);
            auto reading = tx->ExecRequest(srSelect);
            UNIT_ASSERT(reading);
            UNIT_ASSERT(reading->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(reading->GetAffectedRows(), 4);
            UNIT_ASSERT_VALUES_EQUAL(readingRecords.size(), 4);
            auto read = readingRecords.front();
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("snapshot_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("object_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("data"), "data 1");

            TRecordsSetWT readingRecords2;
            TSRSelect srSelect2(table, &readingRecords2);
            srSelect2.InitCondition<TSRBinary>("object_id", 2, ESRBinary::Greater);
            auto readingWithCondition = tx->ExecRequest(srSelect2);
            UNIT_ASSERT(readingWithCondition->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(readingWithCondition->GetAffectedRows(), 2);
            UNIT_ASSERT_VALUES_EQUAL(readingRecords2.size(), 2);
            auto read2 = readingRecords2.front();
            UNIT_ASSERT_VALUES_EQUAL(read2.GetString("snapshot_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(read2.GetString("object_id"), "3");
            UNIT_ASSERT_VALUES_EQUAL(read2.GetString("data"), "data 3");

            UNIT_ASSERT(tx->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorGetRows)

    Y_UNIT_TEST(TableAccessorRemoveRow) {
        constexpr const char* table = "TableAccessorRemoveRow";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(true);
            TRecordsSetWT removeRecords;
            TSRDelete srDelete(table, &removeRecords);
            auto removeRows = tx->ExecRequest(srDelete.InitCondition<TSRBinary>("object_id", 3));
            UNIT_ASSERT(removeRows);
            UNIT_ASSERT(removeRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(removeRows->GetAffectedRows(), 1);
            UNIT_ASSERT_VALUES_EQUAL(removeRecords.size(), 1);
            auto read = removeRecords.front();
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("snapshot_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("object_id"), "3");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("data"), "data 3");

            UNIT_ASSERT(tx->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorRemoveRow)

    Y_UNIT_TEST(TableAccessorUpdateRow) {
        constexpr const char* table = "TableAccessorUpdateRow";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);

            TRecordsSetWT updateRecords;
            TSRUpdate srUpdate(table, &updateRecords);
            {
                srUpdate.InitUpdate<TSRBinary>("data", "new data");
                auto& srMulti = srUpdate.RetCondition<TSRMulti>();
                srMulti.InitNode<TSRBinary>("snapshot_id", 1);
                srMulti.InitNode<TSRBinary>("object_id", 1);
            }
            auto updateRows = tx->ExecRequest(srUpdate);
            UNIT_ASSERT(updateRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(updateRows->GetAffectedRows(), 1);
            UNIT_ASSERT_VALUES_EQUAL(updateRecords.size(), 1);
            auto updated = updateRecords.front();
            UNIT_ASSERT_VALUES_EQUAL(updated.GetString("snapshot_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(updated.GetString("object_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(updated.GetString("data"), "data 1");
            UNIT_ASSERT(tx->Commit());
        }
        {
            auto tx = db->CreateTransaction(false);
            TRecordsSetWT readingRecords;
            TSRSelect srSelect(table, &readingRecords);
            srSelect.InitCondition<TSRBinary>("object_id", 1);
            auto reading = tx->ExecRequest(srSelect);
            UNIT_ASSERT(reading->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(reading->GetAffectedRows(), 1);
            UNIT_ASSERT_VALUES_EQUAL(readingRecords.size(), 1);
            auto read = readingRecords.front();
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("snapshot_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("object_id"), "1");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("data"), "new data");
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorUpdateRow)

    Y_UNIT_TEST(TableAccessorAddRow) {
        constexpr const char* table = "TableAccessorAddRow";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);

            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);

            NSQL::TTableRecord inserted;
            inserted.Set("snapshot_id", 2);
            inserted.Set("object_id", 8);
            inserted.Set("data", "data 8");

            TRecordsSetWT addRowRecords;
            auto addRow = tableAccessor->AddRow(inserted, tx, "snapshot_id == 2", &addRowRecords);
            UNIT_ASSERT(addRow);
            UNIT_ASSERT(addRow->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(addRow->GetAffectedRows(), 1);
            UNIT_ASSERT_VALUES_EQUAL(addRowRecords.size(), 1);
            auto read = addRowRecords.front();
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("snapshot_id"), "2");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("object_id"), "8");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("data"), "data 8");

            UNIT_ASSERT(tx->Commit());

            auto tx2 = db->CreateTransaction(false);
            TRecordsSetWT removeRecords;
            TSRDelete srDelete(table, &removeRecords);
            auto removeRows = tx2->ExecRequest(srDelete.InitCondition<TSRBinary>("object_id", 8));
            UNIT_ASSERT(removeRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(removeRows->GetAffectedRows(), 1);
            UNIT_ASSERT_VALUES_EQUAL(removeRecords.size(), 1);
            auto read2 = removeRecords.front();
            UNIT_ASSERT_VALUES_EQUAL(read2.GetString("snapshot_id"), "2");
            UNIT_ASSERT_VALUES_EQUAL(read2.GetString("object_id"), "8");
            UNIT_ASSERT_VALUES_EQUAL(read2.GetString("data"), "data 8");

            UNIT_ASSERT(tx2->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorAddRow)

    Y_UNIT_TEST(TableAccessorAddRows) {
        constexpr const char* table = "TableAccessorAddRows";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);

            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);

            NSQL::TTableRecord inserted;
            inserted.Set("snapshot_id", "4");
            inserted.Set("object_id", "9");
            inserted.Set("data", "data 9");

            NSQL::TTableRecord inserted2;
            inserted2.Set("snapshot_id", "4");
            inserted2.Set("object_id", "10");
            const TString str = "@@data 10' \n\"'";
            inserted2.SetBytes("data", str);

            TRecordsSet recordsSet;
            recordsSet.AddRow(inserted);
            recordsSet.AddRow(inserted2);

            TRecordsSetWT addRowRecords;
            auto addRows = tableAccessor->AddRows(recordsSet, tx, "snapshot_id == 4", &addRowRecords);
            UNIT_ASSERT(addRows);
            UNIT_ASSERT(addRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(addRows->GetAffectedRows(), 2);
            UNIT_ASSERT_VALUES_EQUAL(addRowRecords.size(), 2);
            TSet<TString> wroteLines = {"4,9,data 9", "4,10," + str};
            for (auto&& i : addRowRecords) {
                const TString testLine = i.GetString("snapshot_id") + "," + i.GetString("object_id") + "," + i.GetString("data");
                wroteLines.erase(testLine);
            }
            UNIT_ASSERT_VALUES_EQUAL(wroteLines.size(), 0);

            UNIT_ASSERT(tx->Commit());

            auto tx2 = db->CreateTransaction(false);
            UNIT_ASSERT(tx2);

            TRecordsSetWT removeRecords;
            TSRDelete srDelete(table, &removeRecords);
            auto removeRows = tx2->ExecRequest(srDelete.InitCondition<TSRBinary>("snapshot_id", 4));
            UNIT_ASSERT(removeRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(removeRows->GetAffectedRows(), 2);

            UNIT_ASSERT(tx2->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorAddRows)

    /*
    Y_UNIT_TEST(TableAccessorBulkUpsertRows) {
        constexpr const char* table = "TableAccessorBulkUpsertRows";
        for (ui32 batchSize = 5'000'000; batchSize < 5'000'001; batchSize += 50000) {
            const ui32 batchCount = 1;//'000'000 / batchSize;
            auto db = CreateAndFillDb(table);
            const auto start = Now();
            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);
            TDuration procTime = TDuration::Zero();
            for (ui32 b = 0; b < batchCount; ++b) {
                auto sProc = Now();
                TRecordsSet recordsSet;
                for (ui32 i = 0; i < batchSize; ++i) {
                    NSQL::TTableRecord inserted;
                    inserted.Set("snapshot_id", b + 100);
                    inserted.Set("object_id", RandomNumber<ui64>());
                    inserted.Set("data", "data " + ToString(i));
                    recordsSet.AddRow(inserted);
                }
                procTime += Now() - sProc;
                auto addRows = tableAccessor->BulkUpsertRows(recordsSet, *db);
                UNIT_ASSERT(addRows);
                UNIT_ASSERT(addRows->IsSucceed());
                UNIT_ASSERT_VALUES_EQUAL(addRows->GetAffectedRows(), recordsSet.size());
            }
            Cerr << "Count: " << batchSize << ", Time: " << (Now() - start - procTime) << " + " << procTime << Endl;
//            UNIT_ASSERT(db->DropTable(table));
        }
    } // Y_UNIT_TEST(TableAccessorAddRows)
*/
    Y_UNIT_TEST(TableAccessorAddRowsReturnPacked) {
        constexpr const char* table = "TableAccessorAddRowsReturnPacked";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);

            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);

            NSQL::TTableRecord inserted;
            inserted.Set("snapshot_id", 5);
            inserted.Set("object_id", 11);
            inserted.Set("data", "data 11");

            NSQL::TTableRecord inserted2;
            inserted2.Set("snapshot_id", 5);
            inserted2.Set("object_id", 12);
            inserted2.Set("data", "data 12");

            TRecordsSet recordsSet;
            recordsSet.AddRow(inserted);
            recordsSet.AddRow(inserted2);

            NStorage::TPackedRecordsSet addRowRecords;
            auto addRows = tableAccessor->AddRows(recordsSet, tx, "snapshot_id == 5", &addRowRecords);
            UNIT_ASSERT(addRows);
            UNIT_ASSERT(addRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(addRows->GetAffectedRows(), 2);
            TStringStream ss;
            for (const auto& it: addRowRecords) {
                for (const auto& it2: it) {
                    if (&it2 != it.begin()) {
                        ss << " ";
                    }
                    ss << TString(it2);
                }
                ss << Endl;
            }

            TString correct = R"(correct need to be writed here)";

            UNIT_ASSERT(tx->Commit());

            auto tx2 = db->CreateTransaction(false);
            UNIT_ASSERT(tx2);

            TRecordsSetWT removeRecords;
            TSRDelete srDelete(table, &removeRecords);
            auto removeRows = tx2->ExecRequest(srDelete.InitCondition<TSRBinary>("snapshot_id", 5));
            UNIT_ASSERT(removeRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(removeRows->GetAffectedRows(), 2);

            UNIT_ASSERT(tx2->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorAddRowsReturnPacked)

    Y_UNIT_TEST(TableAccessorAddRowIfNotExists) {
        constexpr const char* table = "TableAccessorAddRowIfNotExists";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);

            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);

            NSQL::TTableRecord inserted;
            inserted.Set("snapshot_id", 6);
            inserted.Set("object_id", 13);
            inserted.Set("data", "data 13");

            TRecordsSetWT addRowRecords;
            auto addRow = tableAccessor->AddIfNotExists(inserted, tx, inserted, &addRowRecords);
            UNIT_ASSERT(addRow);
            UNIT_ASSERT(addRow->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(addRow->GetAffectedRows(), 1);
            UNIT_ASSERT_VALUES_EQUAL(addRowRecords.size(), 1);
            auto read = addRowRecords.front();
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("snapshot_id"), "6");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("object_id"), "13");
            UNIT_ASSERT_VALUES_EQUAL(read.GetString("data"), "data 13");

            UNIT_ASSERT(tx->Commit());

            auto tx2 = db->CreateTransaction(false);
            UNIT_ASSERT(tx2);

            TRecordsSetWT removeRecords;
            TSRDelete srDelete(table, &removeRecords);
            auto removeRows = tx2->ExecRequest(srDelete.InitCondition<TSRBinary>("snapshot_id", 6));
            UNIT_ASSERT(removeRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(removeRows->GetAffectedRows(), 1);

            UNIT_ASSERT(tx2->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorAddRowIfNotExists)

    Y_UNIT_TEST(TableAccessorAddRowsIfNotExists) {
        constexpr const char* table = "TableAccessorAddRowsIfNotExists";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);

            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);

            NSQL::TTableRecord inserted;
            inserted.Set("snapshot_id", 222);
            inserted.Set("object_id", 222);
            inserted.Set("data", "222");

            NSQL::TTableRecord inserted2;
            inserted2.Set("snapshot_id", 222);
            inserted2.Set("object_id", 223);
            inserted2.Set("data", "223");

            TRecordsSet recordsSet;
            recordsSet.AddRow(inserted);
            recordsSet.AddRow(inserted2);

            TRecordsSetWT addRowRecords;
            auto AddRows = tableAccessor->AddIfNotExists(recordsSet, tx, inserted, &addRowRecords);
            UNIT_ASSERT(AddRows);
            UNIT_ASSERT(AddRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(AddRows->GetAffectedRows(), 2);

            UNIT_ASSERT(tx->Commit());

            auto tx2 = db->CreateTransaction(false);
            UNIT_ASSERT(tx2);

            TRecordsSetWT removeRecords;
            TSRDelete srDelete(table, &removeRecords);
            auto removeRows = tx2->ExecRequest(srDelete.InitCondition<TSRBinary>("snapshot_id", 222));
            UNIT_ASSERT(removeRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(removeRows->GetAffectedRows(), 2);

            UNIT_ASSERT(tx2->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorAddRowsIfNotExists)

    Y_UNIT_TEST(TableAccessorUpsert) {
        constexpr const char* table = "TableAccessorUpsert";
        auto db = CreateAndFillDb(table);
        {
            auto tx = db->CreateTransaction(false);

            auto tableAccessor = db->GetTable(table);
            UNIT_ASSERT(tableAccessor);

            NSQL::TTableRecord inserted;
            inserted.Set("snapshot_id", 333);
            inserted.Set("object_id", 333);
            inserted.Set("data", "333");

            //AddIfNotExists(record,  transaction,  unique, recordsSet) {
            TRecordsSetWT addRowRecords;
            bool isUpdated;
            auto addRow = tableAccessor->Upsert(inserted, tx, inserted, &isUpdated, &addRowRecords);
            UNIT_ASSERT(addRow);
            UNIT_ASSERT(addRow->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(addRow->GetAffectedRows(), 1);
            UNIT_ASSERT(!isUpdated);

            UNIT_ASSERT(tx->Commit());

            auto tx2 = db->CreateTransaction(false);
            UNIT_ASSERT(tx2);

            TRecordsSetWT removeRecords;
            TSRDelete srDelete(table, &removeRecords);
            auto removeRows = tx2->ExecRequest(srDelete.InitCondition<TSRBinary>("snapshot_id", 333, ESRBinary::GreaterOrEqual));
            UNIT_ASSERT(removeRows->IsSucceed());
            UNIT_ASSERT_VALUES_EQUAL(removeRows->GetAffectedRows(), 1);

            UNIT_ASSERT(tx2->Commit());
        }
        UNIT_ASSERT(db->DropTable(table));
    } // Y_UNIT_TEST(TableAccessorUpsert)

    Y_UNIT_TEST(GetTableNames) {
        constexpr const char* table = "GetTableNames";
        auto db = CreateAndFillDb(table);
        {
            auto names = db->GetAllTableNames();
            auto it = std::find(names.begin(), names.end(), table);
            UNIT_ASSERT(it);
        }
        UNIT_ASSERT(db->DropTable(table));
    }

    Y_UNIT_TEST(Lock) {
        constexpr const char* table = "Lock";
        auto db = CreateAndFillDb(table);
        {
            auto lock = db->Lock("magrationLock", true, TDuration({1, 0}));
            UNIT_ASSERT(lock);
        }
        UNIT_ASSERT(db->DropTable(table));
    }

    Y_UNIT_TEST(DropTable) {
        constexpr const char* table = "DropTable";
        auto db = CreateAndFillDb(table);
        {
            UNIT_ASSERT(db->DropTable(table));
        }
    } // Y_UNIT_TEST(DropTable)
} // Y_UNIT_TEST_SUITE(YDBTestSuite)
