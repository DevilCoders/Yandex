#include "props.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NUgc {
    class TRowTest: public TTestBase {
    public:
        UNIT_TEST_SUITE(TRowTest)
        UNIT_TEST(SetsAndGetsProps)
        UNIT_TEST(ReplacesProps)
        UNIT_TEST(RemovesProp)
        UNIT_TEST(ThrowsOnKeyProp)
        UNIT_TEST(Serializes)
        UNIT_TEST(Deserializes)
        UNIT_TEST(DeserializesFromEmptyDict)
        UNIT_TEST(DeserializesWithNonStringProps)
        UNIT_TEST(ThrowsOnDeserializationFromArrayJson)
        UNIT_TEST(ThrowsOnDeserializationFromStringJson)
        UNIT_TEST(ThrowsOnDeserializationFromNonSimpleProps)
        UNIT_TEST(Merges)
        UNIT_TEST_SUITE_END();

        void SetsAndGetsProps() {
            TVirtualRow row;
            UNIT_ASSERT(!row.HasProp("name"));
            UNIT_ASSERT(row.GetProp("name").empty());
            row.SetProp("name", "value");
            UNIT_ASSERT(row.HasProp("name"));
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("name"), "value");
        }

        void ReplacesProps() {
            TVirtualRow row;
            row.SetProp("name", "value");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("name"), "value");

            row.SetProp("name", "other value");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("name"), "other value");
        }

        void RemovesProp() {
            TVirtualRow row;
            row.SetProp("name", "value");
            UNIT_ASSERT(row.HasProp("name"));
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("name"), "value");

            row.RemoveProp("name");
            UNIT_ASSERT(!row.HasProp("name"));
            UNIT_ASSERT(row.GetProp("name").empty());
        }

        void ThrowsOnKeyProp() {
            TVirtualRow row;
            UNIT_ASSERT_EXCEPTION(row.SetProp("key", "value"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(row.GetProp("key"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(row.RemoveProp("key"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(row.HasProp("key"), TBadArgumentException);
        }

        void Serializes() {
            TVirtualRow row;
            row.SetProp("name1", "value1");
            row.SetProp("name2", "value2");
            row.SetProp("name3", "value3");
            row.RemoveProp("name3");
            const NSc::TValue json = row.ToJson();
            UNIT_ASSERT(json.IsDict());
            const NSc::TDict& dict = json.GetDict();
            UNIT_ASSERT_VALUES_EQUAL(dict.size(), 3);
            UNIT_ASSERT_STRINGS_EQUAL(dict.Get("name1").GetString(), "value1");
            UNIT_ASSERT_STRINGS_EQUAL(dict.Get("name2").GetString(), "value2");
            UNIT_ASSERT(dict.Get("name3").GetString().empty());
            UNIT_ASSERT(dict.Get("name1").IsString());
            UNIT_ASSERT(dict.Get("name2").IsString());
            UNIT_ASSERT(dict.Get("name3").IsString());
        }

        void Deserializes() {
            NSc::TValue val;
            val["prop_key"] = "prop_val";
            val["key"] = "row_key";
            const TVirtualRow row = TVirtualRow::FromJson(val);
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("prop_key"), "prop_val");
            UNIT_ASSERT_EXCEPTION(row.GetProp("key"), TBadArgumentException);

            UNIT_ASSERT_STRINGS_EQUAL(row.ToJson()["prop_key"].GetString(), "prop_val");
            UNIT_ASSERT(!row.ToJson().Has("key"));
        }

        void DeserializesFromEmptyDict() {
            UNIT_ASSERT_NO_EXCEPTION(TVirtualRow::FromJson("{}"));
            NSc::TValue val;
            val.SetDict();
            UNIT_ASSERT_NO_EXCEPTION(TVirtualRow::FromJson(val));
        }

        void DeserializesWithNonStringProps() {
            NSc::TValue val;
            val["key1"] = 1;
            val["key2"] = 3.14;
            val["key3"] = true;
            const TVirtualRow row = TVirtualRow::FromJson(val);
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("key1"), "1");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("key2"), "3.14");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("key3"), "1");
        }

        void ThrowsOnDeserializationFromArrayJson() {
            NSc::TValue val;
            val.SetArray();
            UNIT_ASSERT_EXCEPTION(TVirtualRow::FromJson(val), TBadArgumentException);
        }

        void ThrowsOnDeserializationFromStringJson() {
            NSc::TValue val;
            val = "str";
            UNIT_ASSERT_EXCEPTION(TVirtualRow::FromJson(val), TBadArgumentException);
        }

        void ThrowsOnDeserializationFromNonSimpleProps() {
            NSc::TValue goodVal;
            goodVal["prop_key"] = "prop_val";
            UNIT_ASSERT_NO_EXCEPTION(TVirtualRow::FromJson(goodVal));

            {
                NSc::TValue val = goodVal;
                val["other_prop"]["dict_subprop"] = "str";
                UNIT_ASSERT_EXCEPTION(TVirtualRow::FromJson(val), TBadArgumentException);
            }

            {
                NSc::TValue val = goodVal;
                val["other_prop"].Push("str");
                UNIT_ASSERT_EXCEPTION(TVirtualRow::FromJson(val), TBadArgumentException);
            }
        }

        void Merges() {
            TVirtualRow row1;
            row1.SetProp("name1", "value1");
            row1.SetProp("name3", "value3");

            TVirtualRow row2;
            row2.SetProp("name2", "value2");
            row1.SetProp("name3", "trololo");

            row1.Merge(row2);

            UNIT_ASSERT_STRINGS_EQUAL(row1.GetProp("name1"), "value1");
            UNIT_ASSERT_STRINGS_EQUAL(row1.GetProp("name2"), "value2");
            UNIT_ASSERT_STRINGS_EQUAL(row1.GetProp("name3"), "trololo");
        }
    };

    class TTableTest: public TTestBase {
    public:
        UNIT_TEST_SUITE(TTableTest)
        UNIT_TEST(CreatesRow)
        UNIT_TEST(GetRows)
        UNIT_TEST(NonexistentRowIsOk)
        UNIT_TEST(RemovesRow)
        UNIT_TEST(Serializes)
        UNIT_TEST(DeserializesFromEmptyArray)
        UNIT_TEST(ThrowsOnDeserializationFromNonArrayJson)
        UNIT_TEST(ThrowsOnDeserializationWithNonStringKey)
        UNIT_TEST(Deserializes)
        UNIT_TEST(MergesRowsWithSameKeyOnDeserialization)
        UNIT_TEST_SUITE_END();

        void CreatesRow() {
            TVirtualTable table;
            table.MutableRow("key").SetProp("name", "value");
            const TVirtualRow& row = table.GetRow("key");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("name"), "value");
        }

        void GetRows() {
            TVirtualTable table;
            table.MutableRow("key").SetProp("name", "value");
            UNIT_ASSERT_VALUES_EQUAL(table.GetRows().size(), 1);
            for (const auto& kv : table.GetRows()) {
                const TString& key = kv.first;
                UNIT_ASSERT_STRINGS_EQUAL(key, "key");
                const NUgc::TVirtualRow& row = kv.second;
                UNIT_ASSERT(row.HasProp("name"));
                const TString& prop = row.GetProp("name");
                UNIT_ASSERT_STRINGS_EQUAL(prop, "value");
            }
        }

        void NonexistentRowIsOk() {
            TVirtualTable table;
            UNIT_ASSERT_NO_EXCEPTION(table.GetRow("row"));
            UNIT_ASSERT(!table.HasRow("row"));

            const NSc::TValue val = table.ToJson();
            UNIT_ASSERT(val.IsArray());
            UNIT_ASSERT_VALUES_EQUAL(val.GetArray().size(), 0);
        }

        void RemovesRow() {
            TVirtualTable table;
            table.MutableRow("row");
            UNIT_ASSERT(table.HasRow("row"));

            table.RemoveRow("row");
            UNIT_ASSERT(!table.HasRow("row"));

            UNIT_ASSERT_NO_EXCEPTION(table.RemoveRow("nonexistent_row"));

            const NSc::TValue val = table.ToJson();
            UNIT_ASSERT(val.IsArray());
            UNIT_ASSERT_VALUES_EQUAL(val.GetArray().size(), 0);
        }

        void Serializes() {
            TVirtualTable table;
            table.MutableRow("a").SetProp("x", "y").SetProp("1", "2");
            table.MutableRow("b"); // empty

            const NSc::TValue json = table.ToJson();
            UNIT_ASSERT(json.IsArray());
            const NSc::TValue& a = json[0];
            const NSc::TValue& b = json[1];

            UNIT_ASSERT_STRINGS_EQUAL(a["key"].GetString(), "a");
            UNIT_ASSERT_STRINGS_EQUAL(a["x"].GetString(), "y");
            UNIT_ASSERT_STRINGS_EQUAL(a["1"].GetString(), "2");
            UNIT_ASSERT_VALUES_EQUAL(a.GetDict().size(), 3);

            UNIT_ASSERT_STRINGS_EQUAL(b["key"].GetString(), "b");
            UNIT_ASSERT_VALUES_EQUAL(b.GetDict().size(), 1);
        }

        void DeserializesFromEmptyArray() {
            UNIT_ASSERT_NO_EXCEPTION(TVirtualTable::FromJson("[]"));
            UNIT_ASSERT_NO_EXCEPTION(TVirtualTable::FromJson(NSc::TValue().SetArray()));
        }

        void ThrowsOnDeserializationFromNonArrayJson() {
            UNIT_ASSERT_EXCEPTION(TVirtualTable::FromJson(NSc::TValue()), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TVirtualTable::FromJson(NSc::TValue().SetDict()), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TVirtualTable::FromJson(NSc::TValue().SetString()), TBadArgumentException);
        }

        void ThrowsOnDeserializationWithNonStringKey() {
            NSc::TValue arr;
            arr.Push()["key"] = 42;
            UNIT_ASSERT_EXCEPTION(TVirtualTable::FromJson(arr), TBadArgumentException);
        }

        void Deserializes() {
            NSc::TValue arr;
            NSc::TValue& row1 = arr.Push();
            row1["key"] = "row1";
            row1["x"] = "y";

            NSc::TValue& row2 = arr.Push();
            row2["key"] = "row2";
            row2["x"] = "x";
            row2["name"] = "value";

            const TVirtualTable table = TVirtualTable::FromJson(arr);
            const TVirtualRow& r1 = table.GetRow("row1");
            const TVirtualRow& r2 = table.GetRow("row2");

            UNIT_ASSERT_STRINGS_EQUAL(r1.GetProp("x"), "y");

            UNIT_ASSERT_STRINGS_EQUAL(r2.GetProp("x"), "x");
            UNIT_ASSERT_STRINGS_EQUAL(r2.GetProp("name"), "value");

            UNIT_ASSERT_VALUES_EQUAL(arr, table.ToJson());
        }

        void MergesRowsWithSameKeyOnDeserialization() {
            NSc::TValue arr;

            NSc::TValue& r1 = arr.Push();
            r1["key"] = "k";
            r1["prop1"] = "p";
            r1["prop"] = "1";

            NSc::TValue& r2 = arr.Push();
            r2["key"] = "k";
            r2["prop2"] = "d";
            r2["prop"] = "2";

            const TVirtualTable table = TVirtualTable::FromJson(arr);
            const auto& row = table.GetRow("k");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("prop"), "2");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("prop1"), "p");
            UNIT_ASSERT_STRINGS_EQUAL(row.GetProp("prop2"), "d");
        }
    };

    class TTablesTest: public TTestBase {
    public:
        UNIT_TEST_SUITE(TTablesTest)
        UNIT_TEST(CreatesTable)
        UNIT_TEST(GetsTable)
        UNIT_TEST(RemovesTable)
        UNIT_TEST(ReservedNames)
        UNIT_TEST(Serializes)
        UNIT_TEST(Deserializes)
        UNIT_TEST(DeserializesFromEmptyDict)
        UNIT_TEST(ThrowsOnDeserializationFromNonDict)
        UNIT_TEST_SUITE_END();

        void CreatesTable() {
            TVirtualTables tables;
            UNIT_ASSERT(!tables.HasTable("t"));
            UNIT_ASSERT_VALUES_EQUAL(&tables.MutableTable("t"), &tables.MutableTable("t"));
            UNIT_ASSERT(tables.HasTable("t"));
        }

        void GetsTable() {
            TVirtualTables tables;
            UNIT_ASSERT(!tables.HasTable("t"));
            UNIT_ASSERT_NO_EXCEPTION(tables.GetTable("t"));
            UNIT_ASSERT(!tables.HasTable("t"));
        }

        void RemovesTable() {
            TVirtualTables tables;
            UNIT_ASSERT(!tables.HasTable("t"));
            tables.MutableTable("t");
            UNIT_ASSERT(tables.HasTable("t"));
            tables.RemoveTable("t");
            UNIT_ASSERT(!tables.HasTable("t"));
        }

        void ReservedNames() {
            TVirtualTables tables;
            const TString reserved[] = {
                "version",
                "type",
                "userId",
                "visitorId",
                "deviceId",
                "updateId",
                "appId",
                "app",
                "time",
                "timeFrom",
                "timeTo",
            };
            for (const auto& name : reserved) {
                UNIT_ASSERT_EXCEPTION(tables.HasTable(name), TBadArgumentException);
                UNIT_ASSERT_EXCEPTION(tables.GetTable(name), TBadArgumentException);
                UNIT_ASSERT_EXCEPTION(tables.MutableTable(name), TBadArgumentException);
                UNIT_ASSERT_EXCEPTION(tables.RemoveTable(name), TBadArgumentException);
            }
        }

        void Serializes() {
            TVirtualTables tables;
            tables.MutableTable("t").MutableRow("r").SetProp("x", "y");
            tables.MutableTable("a");

            const NSc::TValue json = tables.ToJson();
            UNIT_ASSERT(json.IsDict());

            {
                const NSc::TValue& t = json["t"];
                UNIT_ASSERT(t.IsArray());
                UNIT_ASSERT_STRINGS_EQUAL(t[0]["key"].GetString(), "r");
                UNIT_ASSERT_STRINGS_EQUAL(t[0]["x"].GetString(), "y");
            }

            {
                const NSc::TValue& a = json["a"];
                UNIT_ASSERT(a.IsArray());
                UNIT_ASSERT_VALUES_EQUAL(a.GetArray().size(), 0);
            }
        }

        void Deserializes() {
            NSc::TValue json;
            json["a"].Push()["key"] = "k";
            json["time"] = 146;
            json["version"] = "1.0";
            json["type"] = "ugcdata";
            json["userId"] = "ahaha";
            json["updateId"] = "wow";
            json["appId"] = "app";
            json["timeFrom"] = 92;

            const TVirtualTables tables = TVirtualTables::FromJson(json);
            UNIT_ASSERT(tables.HasTable("a"));
            UNIT_ASSERT(tables.GetTable("a").HasRow("k"));

            UNIT_ASSERT_VALUES_EQUAL(json.Get("a"), tables.ToJson().Get("a"));
        }

        void DeserializesFromEmptyDict() {
            UNIT_ASSERT_NO_EXCEPTION(TVirtualTables::FromJson(NSc::TValue().SetDict()));
        }

        void ThrowsOnDeserializationFromNonDict() {
            UNIT_ASSERT_EXCEPTION(TVirtualTables::FromJson(NSc::TValue()), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TVirtualTables::FromJson(NSc::TValue().SetArray()), TBadArgumentException);
        }
    };

    UNIT_TEST_SUITE_REGISTRATION(TRowTest)
    UNIT_TEST_SUITE_REGISTRATION(TTableTest)
    UNIT_TEST_SUITE_REGISTRATION(TTablesTest)
} // namespace NUgc
