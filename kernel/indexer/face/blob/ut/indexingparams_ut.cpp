#include <kernel/indexer/face/blob/datacontainer.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/charset/wide.h>

using namespace NIndexerCore;

///////////////////////////////////////////////////////////////////////////////

class TIndexingParamsTest : public TTestBase {
    UNIT_TEST_SUITE(TIndexingParamsTest);
        UNIT_TEST(TestDataContainer);
    UNIT_TEST_SUITE_END();

public:
    void TestDataContainer();
};

///////////////////////////////////////////////////////////////////////////////

void TIndexingParamsTest::TestDataContainer() {
    class TInserter : public IIndexDataInserter {
    public:
        void StoreLiteralAttr(const char*, const char*, TPosting) override {
            Cerr << "StoreLiteralAttr" << Endl;
        }
        void StoreLiteralAttr(const char* name, const wchar16* val, TPosting pos) override {
            Cerr << "StoreLiteralAttr(wchar16) = " << name << " " << WideToUTF8(val) << " " << pos << Endl;
        }
        void StoreDateTimeAttr(const char*, time_t) override
        { }
        void StoreIntegerAttr(const char*, const char*, TPosting) override
        { }
        void StoreUrlAttr(const char*, const wchar16*, TPosting) override
        { }
        void StoreKey(const char* name, TPosting pos) override {
            Cerr << "StoreKey = " << name << " " << pos << Endl;
        }
        void StoreZone(const char*, TPosting, TPosting) override
        { }
        void StoreExternalLemma(const wchar16*, size_t, const wchar16*, size_t, ui8, ui8, TPosting) override
        { }
    };

    TBuffer buf1;
    TBuffer buf2;
    TInserter inserter;

    {
        TDataContainer  container;

        container.StoreLiteralAttr("tel_full", "9-000-11-23-451", 5);
        container.StoreLiteralAttr("tel_full", "7-343-18-15-578", 19);
        container.StoreLiteralAttr("text", "some text", 0);
        buf1 = container.SerializeToBuffer();
    }

    //InsertDataTo(TStringBuf(buf1.Data(), buf1.Size()), &inserter, NULL);

    {
        TDataContainer  container;

        container.StoreLiteralAttr("tel_full", "8-800-12-34-567", 16);
        container.StoreKey("text", 23);
        buf2 = container.SerializeToBuffer();
    }

    {
        TVector<TStringBuf> data;
        data.push_back(TStringBuf(buf1.Data(), buf1.Size()));
        data.push_back(TStringBuf(buf2.Data(), buf2.Size()));

        TBuffer res;
        MergeDataContainer(data, &res);

        InsertDataTo(TStringBuf(res.Data(), res.Size()), &inserter, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////

UNIT_TEST_SUITE_REGISTRATION(TIndexingParamsTest);
