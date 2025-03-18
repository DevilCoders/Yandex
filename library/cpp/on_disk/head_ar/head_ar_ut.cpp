#include "head_ar.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/system/tempfile.h>

#include <stdlib.h> // for srand/rand

struct TRec {
    static const ui32 Version = 1;

    ui64 A;
    TRec(ui64 a = 0)
        : A(a)
    {
    }
};

struct TRec2 {
    static const ui32 Version = 1;

    ui64 A;
    ui32 B;

    TRec2(ui64 a = 0, ui32 b = 0xDEAD)
        : A(a)
        , B(b)
    {
    }
};

static const TRec SIMPLE_TEST_DATA[] = {
    TRec(100),
    TRec(200),
    TRec(300),
    TRec(400),
    TRec(500),
    TRec(600),
};

void PrepareData(const TString& path, const TRec* testData, size_t size) {
    TFixedBufferFileOutput out(path);
    TArrayWithHeadWriter<TRec> writer(&out);
    for (size_t i = 0; i < size; ++i) {
        writer.Put(testData[i]);
    }
}

template <class TReader>
void SimpleTest(const TReader& reader) {
    ui32 version = TRec::Version;
    UNIT_ASSERT_VALUES_EQUAL(reader.GetSize(), 6u);
    UNIT_ASSERT_VALUES_EQUAL(reader.GetVersion(), version);
    UNIT_ASSERT_VALUES_EQUAL(reader.GetRecordSize(), sizeof(TRec));
    UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(2).A, 300u);
    UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(5).A, 600u);
}

template <class TReader, class TArrayRec>
void RandomTest(const TReader& reader, const TVector<TRec>& records, size_t size) {
    ui32 version = TRec::Version;
    UNIT_ASSERT_VALUES_EQUAL(reader.GetSize(), size);
    UNIT_ASSERT_VALUES_EQUAL(reader.GetVersion(), version);
    UNIT_ASSERT_VALUES_EQUAL(reader.GetRecordSize(), sizeof(TRec));
    for (size_t i = 0; i < size; ++i) {
        UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(i).A, records[i].A);
    }
}

void RunSimpleTest() {
    TString path = MakeTempName(nullptr, "TArrayWithHeadTest");
    TTempFile deleter(path);

    PrepareData(path, SIMPLE_TEST_DATA, Y_ARRAY_SIZE(SIMPLE_TEST_DATA));

    {
        TArrayWithHead<TRec> reader;
        reader.Load(path.data());
        SimpleTest(reader);
    }

    // Same with polite mode
    {
        TArrayWithHead<TRec2> reader;
        reader.Load(path.data(), /*isPolite*/ true, /*precharge*/ false, /*quiet*/ true);
        SimpleTest(reader);

        // In polite mode, structure will be constructed as vector
        // and should be properly default-initialized. Check this.
        UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(2).B, 0xDEAD);
    }
}

void RunRandomTest() {
    TString path = MakeTempName(nullptr, "TArrayWithHeadTest");
    TTempFile deleter(path);

    srand(42);
    for (size_t attempt = 0; attempt < 10; ++attempt) {
        TVector<TRec> records;

        const size_t size = rand() % 100000 + 500;
        TRec rec;
        for (size_t i = 0; i < size; ++i) {
            rec.A = rand();
            records.push_back(rec);
        }
        PrepareData(path, &records.front(), records.size());
        {
            TArrayWithHead<TRec> reader;
            reader.Load(path.data());
            RandomTest<TArrayWithHead<TRec>, TRec>(reader, records, size);
        }

        // Same with polite mode
        {
            TArrayWithHead<TRec2> reader;
            reader.Load(path.data(), /*isPolite*/ true, /*precharge*/ false, /*quiet*/ true);
            RandomTest<TArrayWithHead<TRec2>, TRec2>(reader, records, size);
        }
    }
}

void SimpleReadWriteTest() {
    TString path = MakeTempName(nullptr, "TArrayWithHeadTest");
    TTempFile deleter(path);

    PrepareData(path, SIMPLE_TEST_DATA, Y_ARRAY_SIZE(SIMPLE_TEST_DATA));

    {
        TMemoryMap memoryMap(path, TMemoryMap::oRdWr);
        TArrayWithHead<TRec> reader;
        reader.Load(memoryMap);
        UNIT_ASSERT(reader.GetIsFile());
        SimpleTest(reader);

        const TRec old = reader.GetAt(5);
        reader[5] = TRec(800);
        UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(5).A, 800);
        reader[5] = TRec(old);
        UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(5).A, old.A);
    }
    // With polite mode updates are still handled, but in memory only
    {
        TMemoryMap memoryMap(path, TMemoryMap::oRdWr);
        TArrayWithHead<TRec2> reader;
        reader.Load(memoryMap, /*isPolite*/ true, /*precharge*/ false, /*quiet*/ true);
        UNIT_ASSERT(!reader.GetIsFile());
        SimpleTest(reader);
        const TRec2 old = reader.GetAt(5);
        reader[5] = TRec2(800, 5);
        UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(5).A, 800);
        UNIT_ASSERT_VALUES_EQUAL(reader.GetAt(5).B, 5);

        TArrayWithHead<TRec2> reader2;
        reader2.Load(path.data(), /*isPolite*/ true, /*precharge*/ false, /*quiet*/ true);
        UNIT_ASSERT_VALUES_EQUAL(reader2.GetAt(5).A, old.A);
        SimpleTest(reader2);
    }
}

Y_UNIT_TEST_SUITE(TArrayWithHeadTest) {
    Y_UNIT_TEST(Simple) {
        RunSimpleTest();
    }

    Y_UNIT_TEST(Random) {
        RunRandomTest();
    }

    Y_UNIT_TEST(SimpleRW) {
        SimpleReadWriteTest();
    }
}
