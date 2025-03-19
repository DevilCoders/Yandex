#include <kernel/common_server/migrations/manager.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/builder.h>
#include <util/system/env.h>
#include <kernel/common_server/library/storage/reply/decoder.h>
#include <kernel/common_server/library/storage/postgres/config.h>
#include <kernel/common_server/library/storage/postgres/table_accessor.h>
#include <kernel/common_server/library/storage/selection/iterator.h>
#include <kernel/common_server/library/storage/selection/multi_iterator.h>
#include <kernel/common_server/library/storage/abstract/config.h>

namespace {
    TString GetConnectionString() {
        TStringBuilder connectionString;
        connectionString << "host=" << GetEnv("POSTGRES_RECIPE_HOST");
        connectionString << " port=" << GetEnv("POSTGRES_RECIPE_PORT");
        connectionString << " target_session_attrs=read-write dbname=" << GetEnv("POSTGRES_RECIPE_DBNAME");
        connectionString << " user=" << GetEnv("POSTGRES_RECIPE_USER");
        connectionString << " password=" << Endl;
/*
        connectionString << "host=" << "sas-waniis89zubqxw1v.db.yandex.net";
        connectionString << " port=6432";
        connectionString << " target_session_attrs=read-write dbname=risk-fintech-tests";
        connectionString << " user=" << "tests-user";
        connectionString << " password=risk-fintech-tests" << Endl;
        */
        return connectionString;
    }

    class TTestObject {
    private:
        CSA_DEFAULT(TTestObject, TString, First);
        CSA_DEFAULT(TTestObject, TString, Second);
        CSA_DEFAULT(TTestObject, TString, Third);
        CSA_DEFAULT(TTestObject, ui32, Index);
    public:
        class TDecoder: public TBaseDecoder {
        private:
            DECODER_FIELD(First);
            DECODER_FIELD(Second);
            DECODER_FIELD(Third);
            DECODER_FIELD(Index);
        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderOriginal) {
                {
                    auto it = decoderOriginal.find("f");
                    CHECK_WITH_LOG(it != decoderOriginal.end());
                    First = it->second;
                }
                {
                    auto it = decoderOriginal.find("s");
                    CHECK_WITH_LOG(it != decoderOriginal.end());
                    Second = it->second;
                }
                {
                    auto it = decoderOriginal.find("t");
                    CHECK_WITH_LOG(it != decoderOriginal.end());
                    Third = it->second;
                }
                {
                    auto it = decoderOriginal.find("idx");
                    CHECK_WITH_LOG(it != decoderOriginal.end());
                    Index = it->second;
                }
            }
        };

        NCS::NStorage::TTableRecord SerializeToTableRecord() const {
            NCS::NStorage::TTableRecord result;
            result.Set("f", First);
            result.Set("s", Second);
            result.Set("t", Third);
            result.SetNotEmpty("index", Index);
            return result;
        }

        bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            READ_DECODER_VALUE(decoder, values, First);
            READ_DECODER_VALUE(decoder, values, Second);
            READ_DECODER_VALUE(decoder, values, Third);
            READ_DECODER_VALUE(decoder, values, Index);
            return true;
        }
    };

    class TTestObjectWide {
    private:
        ui32 Index;
        TString Data;
        CSA_DEFAULT(TTestObjectWide, TTestObject, Object);
    public:
        TTestObjectWide(TTestObject&& object)
            : Object(object)
        {
            Y_UNUSED(Index);
            Y_UNUSED(Data);
        }

        TString GetFirst() const {
            return Object.GetFirst();
        }
        TString GetSecond() const {
            return Object.GetSecond();
        }
        TString GetThird() const {
            return Object.GetThird();
        }
    };

    TDatabasePtr InitDb() {
        NCS::NStorage::TPostgresConfig config;
        config.SetSimpleConnectionString(GetConnectionString());
        TString dbName = TGUID::CreateTimebased().AsUuidString();
        SubstGlobal(dbName, "-", "");
        config.SetMainNamespace("ns_" + dbName).SetUseBalancing(false);
        NCS::NStorage::IDatabaseConstructionContext dcc;
        dcc.SetDBName("db_" + dbName);
        TDatabasePtr db = config.ConstructDatabase(&dcc);
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
        {
            auto tx = db->CreateTransaction();
            auto tableAccessor = db->GetTable("txn_test");
            CHECK_WITH_LOG(tableAccessor);

            {
                TSRDelete srDelete("txn_test");
                CHECK_WITH_LOG(!!tx->ExecRequest(srDelete));
                TSRDelete srDelete1("txn_test1");
                CHECK_WITH_LOG(!!tx->ExecRequest(srDelete1));
            }
            TRecordsSet records;
            records.AddRow(TTestObject().SetFirst("1").SetSecond("a").SetThird("A").SerializeToTableRecord());
            records.AddRow(TTestObject().SetFirst("3").SetSecond("c").SetThird("C").SerializeToTableRecord());
            records.AddRow(TTestObject().SetFirst("5").SetSecond("e").SetThird("E").SerializeToTableRecord());

            CHECK_WITH_LOG(!!tableAccessor->AddRows(records, tx, ""));
            {
                NCS::NStorage::TObjectRecordsSet<TTestObject> result;
                TSRSelect srSelect("txn_test", &result);
                CHECK_WITH_LOG(!!tx->ExecRequest(srSelect));
                CHECK_WITH_LOG(result.size() == 3) << result.size() << Endl;
            }

            CHECK_WITH_LOG(tx->Commit());
        }
        {
            auto tx = db->CreateTransaction();
            auto tableAccessor = db->GetTable("txn_test1");
            CHECK_WITH_LOG(tableAccessor);

            TRecordsSet records;
            records.AddRow(TTestObject().SetFirst("2").SetSecond("b").SetThird("B").SerializeToTableRecord());
            records.AddRow(TTestObject().SetFirst("4").SetSecond("d").SetThird("D").SerializeToTableRecord());
            records.AddRow(TTestObject().SetFirst("6").SetSecond("f").SetThird("F").SerializeToTableRecord());

            CHECK_WITH_LOG(!!tableAccessor->AddRows(records, tx, ""));
            CHECK_WITH_LOG(tx->Commit());
        }

        return db;
    }

}

class TFilter: public NCS::NSelection::NFilter::TObjectIds {
public:
    TFilter() {
        SetObjectIdFieldName("f");
    }
};

class TSorting: public NCS::NSelection::NSorting::TLinear {
public:
    TSorting() {
        RegisterField("f").RegisterField("s");
    }
};

class TSelection: public NCS::NSelection::TFullSelection<TTestObject, TFilter, TSorting> {
public:

};

class TAdaptableReader: public NCS::NSelection::IAdaptiveObjectsReader<TTestObjectWide, TTestObject> {
private:
    using TBase = NCS::NSelection::IAdaptiveObjectsReader<TTestObjectWide, TTestObject>;
protected:
    virtual bool DoAdaptObjects(TVector<TTestObject>&& originalObjects, TVector<TTestObjectWide>& result) const {
        TVector<TTestObjectWide> localResult;
        for (auto&& i : originalObjects) {
            localResult.emplace_back(std::move(TTestObjectWide(std::move(i))));
        }
        std::swap(localResult, result);
        return true;
    }
public:
    using TBase::TBase;
};

const TString kJsonSimpleReaderString = R"( {
    "count_limit": 1,
    "object_id" : ["1", "2", "3", "4", "5", "6"]
})";

const TString kJsonSimpleReaderDescString = R"( {
    "count_limit": 1,
    "object_id" : ["1", "2", "3", "4", "5", "6"],
    "sort_fields" : [{"field_id" : "f", "asc" : false}]
})";

Y_UNIT_TEST_SUITE(Selection) {

    Y_UNIT_TEST(Reader) {
        auto db = InitDb();
        NCS::NSelection::TReader<TTestObject, TSelection> reader("txn_test");
        TVector<TTestObject> objects;
        CHECK_WITH_LOG(reader.DeserializeFromJson(NJson::ReadJsonFastTree(kJsonSimpleReaderString)));
        auto txRead = db->CreateTransaction(true);

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(objects.size() == 1);
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetFirst() == "1");
        CHECK_WITH_LOG(objects.front().GetSecond() == "a");
        CHECK_WITH_LOG(objects.front().GetThird() == "A");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetSecond() == "c");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(!reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "E");
        CHECK_WITH_LOG(reader.Read(*txRead));
        CHECK_WITH_LOG(!reader.GetHasMore());
    }

    Y_UNIT_TEST(ReaderDesc) {
        auto db = InitDb();
        NCS::NSelection::TReader<TTestObject, TSelection> reader("txn_test");
        TVector<TTestObject> objects;
        CHECK_WITH_LOG(reader.DeserializeFromJson(NJson::ReadJsonFastTree(kJsonSimpleReaderDescString)));
        auto txRead = db->CreateTransaction(true);

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(objects.size() == 1);
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetFirst() == "5");
        CHECK_WITH_LOG(objects.front().GetSecond() == "e");
        CHECK_WITH_LOG(objects.front().GetThird() == "E");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetSecond() == "c");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(!reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "A");
        CHECK_WITH_LOG(reader.Read(*txRead));
        CHECK_WITH_LOG(!reader.GetHasMore());
    }

    Y_UNIT_TEST(ReaderCursorDesc) {
        auto db = InitDb();
        NJson::TJsonValue jsonInfo = NJson::ReadJsonFastTree(kJsonSimpleReaderDescString);
        {
            NCS::NSelection::TReader<TTestObject, TSelection> reader("txn_test");
            TVector<TTestObject> objects;
            CHECK_WITH_LOG(reader.DeserializeFromJson(jsonInfo));
            auto txRead = db->CreateTransaction(true);

            CHECK_WITH_LOG(reader.Read(*txRead));
            objects = reader.DetachObjects();
            CHECK_WITH_LOG(objects.size() == 1);
            CHECK_WITH_LOG(reader.GetHasMore());
            CHECK_WITH_LOG(objects.front().GetFirst() == "5");
            CHECK_WITH_LOG(objects.front().GetSecond() == "e");
            CHECK_WITH_LOG(objects.front().GetThird() == "E");

            CHECK_WITH_LOG(reader.Read(*txRead));
            objects = reader.DetachObjects();
            CHECK_WITH_LOG(reader.GetHasMore());
            CHECK_WITH_LOG(objects.front().GetSecond() == "c");
            jsonInfo.InsertValue("cursor", reader.SerializeCursorToString());
        }
        {
            NCS::NSelection::TReader<TTestObject, TSelection> reader("txn_test");
            TVector<TTestObject> objects;
            CHECK_WITH_LOG(reader.DeserializeFromJson(jsonInfo));
            CHECK_WITH_LOG(reader.SerializeCursorToString() == jsonInfo["cursor"].GetString());
            auto txRead = db->CreateTransaction(true);

            CHECK_WITH_LOG(reader.Read(*txRead));
            objects = reader.DetachObjects();
            CHECK_WITH_LOG(!reader.GetHasMore());
            CHECK_WITH_LOG(objects.front().GetThird() == "A");
            CHECK_WITH_LOG(reader.Read(*txRead));
            CHECK_WITH_LOG(!reader.GetHasMore());
        }
    }

    Y_UNIT_TEST(MultiReader) {
        auto db = InitDb();
        NCS::NSelection::TMultiReader<TTestObject, TSelection> reader;
        reader.Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test"));
        reader.Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test1"));
        TVector<TTestObject> objects;
        CHECK_WITH_LOG(reader.DeserializeFromJson(NJson::ReadJsonFastTree(kJsonSimpleReaderString)));
        auto txRead = db->CreateTransaction(true);

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(objects.size() == 1);
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetFirst() == "1");
        CHECK_WITH_LOG(objects.front().GetSecond() == "a");
        CHECK_WITH_LOG(objects.front().GetThird() == "A");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetSecond() == "b");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "C");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "D");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "E");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(!reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "F");
    }

    Y_UNIT_TEST(MultiReaderAdaptable) {
        auto db = InitDb();
        TAtomicSharedPtr<NCS::NSelection::TMultiReader<TTestObject, TSelection>> readerOriginal = new NCS::NSelection::TMultiReader<TTestObject, TSelection>;
        readerOriginal->Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test"));
        readerOriginal->Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test1"));
        TAdaptableReader reader(readerOriginal);
        TVector<TTestObjectWide> objects;
        CHECK_WITH_LOG(reader.DeserializeFromJson(NJson::ReadJsonFastTree(kJsonSimpleReaderString)));
        auto txRead = db->CreateTransaction(true);

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(objects.size() == 1);
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetFirst() == "1");
        CHECK_WITH_LOG(objects.front().GetSecond() == "a");
        CHECK_WITH_LOG(objects.front().GetThird() == "A");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetSecond() == "b");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "C");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "D");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "E");

        CHECK_WITH_LOG(reader.Read(*txRead));
        objects = reader.DetachObjects();
        CHECK_WITH_LOG(!reader.GetHasMore());
        CHECK_WITH_LOG(objects.front().GetThird() == "F");
    }

    Y_UNIT_TEST(MultiReaderCursor) {
        auto db = InitDb();
        NJson::TJsonValue jsonData = NJson::ReadJsonFastTree(kJsonSimpleReaderDescString);
        {
            TAtomicSharedPtr<NCS::NSelection::TMultiReader<TTestObject, TSelection>> readerOriginal = new NCS::NSelection::TMultiReader<TTestObject, TSelection>;
            readerOriginal->Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test"));
            readerOriginal->Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test1"));
            TAdaptableReader reader(readerOriginal);
            TVector<TTestObjectWide> objects;
            CHECK_WITH_LOG(reader.DeserializeFromJson(jsonData));
            reader.SetCountLimit(2);
            auto txRead = db->CreateTransaction(true);

            CHECK_WITH_LOG(reader.Read(*txRead));
            objects = reader.DetachObjects();
            CHECK_WITH_LOG(objects.size() == 2);
            CHECK_WITH_LOG(reader.GetHasMore());
            CHECK_WITH_LOG(objects.front().GetFirst() == "6");
            CHECK_WITH_LOG(objects.front().GetSecond() == "f");
            CHECK_WITH_LOG(objects.front().GetThird() == "F");
            CHECK_WITH_LOG(objects.back().GetFirst() == "5");
            CHECK_WITH_LOG(objects.back().GetSecond() == "e");
            CHECK_WITH_LOG(objects.back().GetThird() == "E");

            CHECK_WITH_LOG(reader.Read(*txRead));
            objects = reader.DetachObjects();
            CHECK_WITH_LOG(objects.size() == 2);
            CHECK_WITH_LOG(reader.GetHasMore());
            CHECK_WITH_LOG(objects.front().GetFirst() == "4");
            CHECK_WITH_LOG(objects.front().GetSecond() == "d");
            CHECK_WITH_LOG(objects.front().GetThird() == "D");
            CHECK_WITH_LOG(objects.back().GetFirst() == "3");
            CHECK_WITH_LOG(objects.back().GetSecond() == "c");
            CHECK_WITH_LOG(objects.back().GetThird() == "C");
            jsonData.InsertValue("cursor", reader.SerializeCursorToString());
        }

        {
            TAtomicSharedPtr<NCS::NSelection::TMultiReader<TTestObject, TSelection>> readerOriginal = new NCS::NSelection::TMultiReader<TTestObject, TSelection>;
            readerOriginal->Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test"));
            readerOriginal->Register(new NCS::NSelection::TReader<TTestObject, TSelection>("txn_test1"));
            TAdaptableReader reader(readerOriginal);
            TVector<TTestObjectWide> objects;
            CHECK_WITH_LOG(reader.DeserializeFromJson(jsonData));
            CHECK_WITH_LOG(reader.SerializeCursorToString() == jsonData["cursor"].GetString());
            reader.SetCountLimit(2);
            auto txRead = db->CreateTransaction(true);

            CHECK_WITH_LOG(reader.Read(*txRead));
            objects = reader.DetachObjects();
            CHECK_WITH_LOG(objects.size() == 2);
            CHECK_WITH_LOG(!reader.GetHasMore());
            CHECK_WITH_LOG(objects.front().GetFirst() == "2");
            CHECK_WITH_LOG(objects.front().GetSecond() == "b");
            CHECK_WITH_LOG(objects.front().GetThird() == "B");
            CHECK_WITH_LOG(objects.back().GetFirst() == "1");
            CHECK_WITH_LOG(objects.back().GetSecond() == "a");
            CHECK_WITH_LOG(objects.back().GetThird() == "A");
            CHECK_WITH_LOG(!reader.SerializeCursorToString());
        }
    }
}
