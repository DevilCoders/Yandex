#include "table.h"
#include "client.h"
#include "ut/lib.h"

#include <util/datetime/base.h>
#include <util/generic/guid.h>

namespace NYT {
namespace NProtoApiTest {

class TTestTable : public TTestBase
{
public:
    template<class TProto, class TClient>
    TTable<TProto> Table(
        TString name, TClient client, TTableOptions const& opts = {})
    {
        return TTable<TProto>(BasePath + "/" + name, client, opts);
    }

    void SetUp() override {
        Client = NYT::NApi::CreateClient(GetEnv("YT_PROXY"));
        BasePath = GetEnv("YT_PREFIX") + "/" + CreateGuidAsString();
        {
            NApi::TCreateNodeOptions opts;
            opts.Recursive = true;
            Client->CreateNode(BasePath, NObjectClient::EObjectType::MapNode, opts)
                .Get().ValueOrThrow();
        }
        Zoo = Table<TZoo>("zoo", Client);
    }

    TZoo Prelude() {
        Zoo.Create();
        Zoo.Mount();

        //
        // Read (precondition)
        //
        TZoo row;
        row.SetSigned(1);
        row.SetSigned32(2);
        ASSERT_EQ(Zoo.LookupRows(row).size(), 0);

        //
        // Write
        //
        row.SetUnsigned(42);
        row.SetUnsigned32(123);
        row.SetFloat(1.0);
        row.SetFloatDelta(42.0);
        row.SetString("test");
        row.MutableBox()->SetBar("foo");

        Transaction = StartTransaction(Client);
        Zoo.In(Transaction).WriteRows(row);
        Commit(Transaction);
        row.SetBool(true); // expression (1 % 2 = 1) -> true

        //
        // Read (postcondition)
        //
        row.SetFloat(42.0);
        ASSERT_PROTO_EQ(Zoo.LookupRows(row)[0], row);

        return row;
    }

    void TearDown() override {
        NApi::TRemoveNodeOptions opts;
        opts.Recursive = true;
        Client->RemoveNode(BasePath, opts)
            .Get().ThrowOnError();
    }

    void TestManage();
    void TestAutoShard();
    void TestReshard();
    void TestCheckSchema();
    void TestCRUD();
    void TestLookupFiltered();
    void TestPushSelect();
    //void TestComplexSelect();

    UNIT_TEST_SUITE(TTestTable)
        UNIT_TEST(TestManage);
        UNIT_TEST(TestAutoShard);
        UNIT_TEST(TestReshard);
        UNIT_TEST(TestCheckSchema);
        UNIT_TEST(TestCRUD);
        UNIT_TEST(TestLookupFiltered);
        UNIT_TEST(TestPushSelect);
        //UNIT_TEST(TestComplexSelect);
    UNIT_TEST_SUITE_END();

    NApi::IClientPtr Client;
    NApi::ITransactionPtr Transaction;
    TString BasePath;
    TTable<TZoo> Zoo;
};

UNIT_TEST_SUITE_REGISTRATION(TTestTable);

void TTestTable::TestManage() {
    ASSERT_FALSE(Zoo.Exists());
    Zoo.Create();
    ASSERT_TRUE(Zoo.Exists());
    ASSERT_TRUE(Zoo.GetState() == ETabletState::Unmounted);
    Zoo.Mount();
    ASSERT_TRUE(Zoo.GetState() == ETabletState::Mounted);
    Zoo.Freeze();
    ASSERT_TRUE(Zoo.GetState() == ETabletState::Frozen);
    Zoo.Unmount();
    ASSERT_TRUE(Zoo.GetState() == ETabletState::Unmounted);
}

void TTestTable::TestAutoShard() {
    Zoo.Create();
    Zoo.Reshard(17);
    Zoo.Mount();
    auto pivotKeys = Zoo.PivotKeys();
    ASSERT_EQ(17, pivotKeys.size());
    for (const TZoo& key : pivotKeys) {
        Cout << key.Utf8DebugString();
    }
}

void TTestTable::TestReshard() {
    Zoo.Create();

    auto makeKey2 = [](i64 a, i32 b) {
        TZoo key;
        key.SetSigned(a);
        key.SetSigned32(b);
        return key;
    };

    auto makeKey1 = [](i64 a) {
        TZoo key;
        key.SetSigned(a);
        return key;
    };

    SetYtAttr(Zoo.GetClientBase(), Zoo.GetPath(), "enable_tablet_balancer", false);

    TVector<TZoo> pivotKeys;
    pivotKeys.push_back(TZoo());
    pivotKeys.push_back(makeKey2(-1000, -500));
    pivotKeys.push_back(makeKey2(-1000, 0));
    pivotKeys.push_back(makeKey1(0));
    Zoo.Reshard(pivotKeys);
    ASSERT_EQ(Zoo.TabletCount(), pivotKeys.size());
    Zoo.Mount();

    TInstant start = TInstant::Now();
    for (size_t i : xrange(10000)) {
        Y_UNUSED(i);
        ASSERT_EQ(Zoo.TabletIndex(makeKey2(-2000, 0)), 0);
        ASSERT_EQ(Zoo.TabletIndex(makeKey2(-1000, -500)), 1);
        ASSERT_EQ(Zoo.TabletIndex(makeKey2(-1000, -499)), 1);
        ASSERT_EQ(Zoo.TabletIndex(makeKey2(-1000, 0)), 2);
        ASSERT_EQ(Zoo.TabletIndex(makeKey1(-500)), 2);
        ASSERT_EQ(Zoo.TabletIndex(makeKey2(0, 0)), 3);
        ASSERT_EQ(Zoo.TabletIndex(makeKey2(1, 0)), 3);
        Cerr << "\rattempt " << i << Flush;
    }
    Cerr << Endl;
    TDuration total = TInstant::Now() - start;
    Cerr << "TabletIndex duration: " << total << Endl;
    // TODO: this may be uncommeted if TabletIndex is cached and we indeed need fast TabletIndex
    // Now it goes to @tablets attribute every time
    // ASSERT_LT(total, TDuration::Seconds(60));
}

void TTestTable::TestCheckSchema() {
    Table<TFilteredZoo>("zoo", Client).Create();
    ASSERT_NO_THROW(Table<TZoo>("zoo", Client));
    TTableOptions opts;
    opts.CheckSchemaStrictly = true;
    ASSERT_THROW(Table<TZoo>("zoo", Client, opts), NYT::TErrorException);
    ASSERT_THROW(Table<TOrdered>("zoo", Client), NYT::TErrorException);
}

void TTestTable::TestCRUD() {
    TZoo row = Prelude();

    //
    // Update with aggregate
    //
    Transaction = StartTransaction(Client);
    row.SetFloatDelta(42.0);
    Zoo.In(Transaction).WriteRows(row);
    row.SetFloat(42.0);
    Commit(Transaction);

    //
    // Read last committed
    //
    {
        TZoo expected = row;
        expected.SetFloat(84.0);
        ASSERT_PROTO_EQ(Zoo.LookupRows(row)[0], expected);
    }

    //
    // Read previous state by timestamp
    //
    {
        TZoo expected = row;
        expected.SetFloat(42.0);
        TTable<TZoo> oldZoo = Zoo.At(Transaction->GetStartTimestamp());
        ASSERT_PROTO_EQ(oldZoo.LookupRows(row)[0], expected);
    }

    //
    // Delete
    //
    Transaction = StartTransaction(Client);
    Zoo.In(Transaction).DeleteRows(row);
    Commit(Transaction);

    // Check postcondition
    ASSERT_EQ(Zoo.LookupRows(row).size(), 0);
}

void TTestTable::TestLookupFiltered() {
    TZoo row = Prelude();

    NYT::NProtoApi::TLookupRowsOptions opts;
    opts.ColumnFilter = {
        TZoo::kSignedFieldNumber, // key
        TZoo::kDoubleFieldNumber,
        TZoo::kUnsigned32FieldNumber, // renamed
        TZoo::kBoxFieldNumber
    };

    TZoo expected;
    expected.SetSigned(row.GetSigned());
    expected.SetUnsigned32(row.GetUnsigned32());
    *expected.MutableBox() = row.GetBox();
    ASSERT_PROTO_EQ(Zoo.LookupRows(row, opts)[0], expected);
}

void TTestTable::TestPushSelect() {
    auto t = Table<TOrdered>("queue", Client);
    t.Create();
    t.Reshard(10);
    t.Mount();

    TVector<TOrdered> rec{2};
    rec[0].SetTablet(0);
    rec[0].SetUnsigned(42);
    rec[0].MutableBox()->SetBar("foo");
    rec[1].SetTablet(1);
    rec[1].SetUnsigned(24);
    rec[1].MutableBox()->SetBar("bar");

    Transaction = StartTransaction(Client);
    t.In(Transaction).WriteRows(rec);
    Commit(Transaction);

    auto resp = t.SelectRows("* from [$(this)] order by [$tablet_index] limit 100")
                 .Rowset;
    Cout << DbgDump(resp) << Endl;
}

#if 0
// XXX: local_yt hungs on this test
void TTestTable::TestComplexSelect() {
    TZoo row = Prelude();

    // XXX: This fails because YT returns uninitialized aggregate flag from query result
    // ASSERT_PROTO_EQ(Zoo.SelectRows("* from [$(this)]").Rowset[0], row);

    static TString query = R"(
        double(t1.Signed) as X, double(t1.x) * 1.5 as Y, t2.Double as Z
        FROM [$(this)] AS t1
        JOIN [$(this)] AS t2
        ON t1.Signed = t2.Signed
    )";

    // Response contains Z = null that doesn't fits to protobuf.
    ASSERT_THROW(Zoo.SelectRows<TPoint2D>(query), NYT::TErrorException);
    Cout << DbgDump(Zoo.SelectRows<TPoint3D>(query).Rowset) << Endl;
}
#endif

} // NProtoApiTest
} // NYT
