#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/sqlite3/sqlite.h>

#include <util/folder/tempdir.h>

using NSQLite::TSQLiteStatement;
using NSQLite::TSQLiteDB;

Y_UNIT_TEST_SUITE(Tests) {
    Y_UNIT_TEST(TestMemoryLeakInExceptionConstruction) {
        TTempDir tmp;
        tmp.DoNotRemove();
        TSQLiteDB db(
            tmp.Path() / "db.sqlite",
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX);
        TSQLiteStatement createSttmnt(db, "CREATE TABLE IF NOT EXISTS ids(id BLOB PRIMARY KEY)");
        createSttmnt.Execute();

        {
            TSQLiteStatement insertSttmnt(db, "INSERT INTO ids(id) VALUES(?)");
            insertSttmnt.Bind(1, "xxx");
            insertSttmnt.Execute();
        }

        bool failed = false;
        try {
            TSQLiteStatement insertSttmnt(db, "INSERT INTO ids(id) VALUES(?)");
            insertSttmnt.Bind(1, "xxx");
            // will raise exception
            insertSttmnt.Execute();
        } catch (const std::exception&) {
            failed = true;
        }

        UNIT_ASSERT(failed);
    }

    Y_UNIT_TEST(TestNegativeIntInColumn) {
        TTempDir tmp;
        tmp.DoNotRemove();
        TSQLiteDB db(
            tmp.Path() / "db.sqlite",
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX);
        TSQLiteStatement(db, R"(
            CREATE TABLE IF NOT EXISTS table_(
                key BLOB PRIMARY KEY,
                value INTEGER NOT NULL
            ))").Execute();
        TSQLiteStatement(db, R"(INSERT INTO table_(key, value) VALUES(?,?))")
            .BindBlob(1, "key")
            .Bind(2, -1)
            .Execute();

        TSQLiteStatement s(db, R"(SELECT value FROM table_ WHERE key = ?)");
        s.BindBlob(1, "key");

        UNIT_ASSERT(s.Step());
        const auto value = s.ColumnInt64(0);
        UNIT_ASSERT(!s.Step());

        UNIT_ASSERT_VALUES_EQUAL(value, -1);
    }
}
