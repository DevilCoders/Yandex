#include <library/cpp/testing/unittest/registar.h>
#include "rdkeyit.h"
#include "oldindexfile.h"
#include "memoryportion.h"

class TIndexFileTest : public TTestBase {
    UNIT_TEST_SUITE(TIndexFileTest);
        UNIT_TEST(TestRawIndex);
        UNIT_TEST(TestSubIndex);
    UNIT_TEST_SUITE_END();
public:
    void TestRawIndex();
    void TestSubIndex();
};

UNIT_TEST_SUITE_REGISTRATION(TIndexFileTest);

void TIndexFileTest::TestRawIndex() {
    NIndexerCore::TRawIndexFile fOut;
    fOut.Open("testkey", "testinv");

    TVector<const char*> hosts;
    hosts.push_back("aaa.com");
    hosts.push_back("bbb.com");
    hosts.push_back("ccc.com");

    for (size_t i = 0; i < hosts.size(); i++) {
        fOut.StoreNextKey(hosts[i]);
        for (int j = 0; j < 5; j++) { // write 5 hits for each host
            fOut.StoreNextHit(1000 + i * 10 + j); // write "normal" hit
            fOut.StoreRaw(j); // write raw1
            fOut.StoreRaw(j + 1); // write raw2
        }
    }

    fOut.CloseEx();

    TReadKeysIterator ri(IYndexStorage::FINAL_FORMAT);
    ri.Init("testkey", "testinv");
    int i = 0;
    while (ri.Next()) {
        UNIT_ASSERT_STRINGS_EQUAL(hosts[i], ri.GetKeyText()); // get key
        int j = 0;
        while (ri.Hits.Next()) { // read hits
            const SUPERLONG expectedHit = 1000 + i * 10 + j;
            UNIT_ASSERT_VALUES_EQUAL(ri.Hits.Current(), expectedHit); // normal hit
            i64 raw1 = 0, raw2 = 0;
            UNIT_ASSERT(ri.Hits.ReadPackedI64(raw1)); // raw1 hit
            UNIT_ASSERT_VALUES_EQUAL(raw1, j);
            UNIT_ASSERT(ri.Hits.ReadPackedI64(raw2)); // raw2 hit
            UNIT_ASSERT_VALUES_EQUAL(raw2, j + 1);
            ++j;
        }
        ++i;
    }

    remove("testkey");
    remove("testinv");
}

struct TTestSubIndexWriter {
    void AddPerst(SUPERLONG pos, ui32 len) {
        UNIT_FAIL("AddPerst( [" << TWordPosition::Doc(pos) << "." << TWordPosition::Break(pos) << "."
            << TWordPosition::Word(pos) << "], " << len << " )");
    }
    void WriteSubIndex(NIndexerCore::TOutputIndexFileImpl<NIndexerCore::NIndexerDetail::TOutputMemoryStream>&,
        i64 count, ui32 length, const TSubIndexInfo& subIndexInfo)
    {
        if (needSubIndex(count, subIndexInfo)) {
            UNIT_FAIL("WriteSubIndex( file, " << count << ", " << length << ", subIndexInfo )");
        }
    }
    void ClearSubIndex() {
    }
};

template <typename TWriter>
inline void WriteHitsAndKey(TWriter& writer, TWordPosition& pos, size_t n, const char* key) {
    UNIT_ASSERT(writer.GetSubIndexInfo().nSubIndexStep < n && writer.GetSubIndexInfo().nMinNeedSize < n);
    while (n--) {
        writer.WriteHit(pos.SuperLong());
        pos.Inc();
    }
    writer.WriteKey(key);
}

void TIndexFileTest::TestSubIndex() {
    using namespace NIndexerCore;
    typedef TOutputIndexFileImpl<NIndexerDetail::TOutputMemoryStream> TTestOutputIndexFile;
    typedef TInvKeyWriterImpl<TTestOutputIndexFile, THitWriterImpl<CHitCoder, NIndexerDetail::TOutputMemoryStream>,
        TTestSubIndexWriter, NIndexerDetail::TFastAccessTableWriter> TTestInvKeyWriter;
    typedef TInputIndexFileImpl<NIndexerDetail::TInputMemoryStream> TTestInputIndexFile;
    typedef TInvKeyReaderImpl<TTestInputIndexFile> TTestInvKeyReader;

    // write index
    NIndexerDetail::TOutputMemoryStream keyStream, invStream;
    const IYndexStorage::FORMAT format = IYndexStorage::FINAL_FORMAT;
    const ui32 version = YNDEX_VERSION_CURRENT;
    TTestOutputIndexFile index(keyStream, invStream, format, version);
    TTestInvKeyWriter writer(index, false); // noSubIndex
    TWordPosition pos(1, 1, 1); // docID, break, word
    WriteHitsAndKey(writer, pos, 1000, "a");
    pos.Set(2, 1, 1);
    WriteHitsAndKey(writer, pos, 2000, "b");
    pos.Set(3, 1, 1);
    WriteHitsAndKey(writer, pos, 3000, "c");
    index.Close(); // don't write FAT

    // read index
    NIndexerDetail::TInputMemoryStream inputKeyStream(keyStream.GetBuffer().Data(), keyStream.GetBuffer().Size());
    NIndexerDetail::TInputMemoryStream inputInvStream(invStream.GetBuffer().Data(), invStream.GetBuffer().Size());
    TTestInputIndexFile inputIndex(inputKeyStream, inputInvStream, format); // version will be read from index
    TTestInvKeyReader reader(inputIndex, false); // noSubIndex

    size_t count = 0;
    UNIT_ASSERT(reader.ReadNext());
//    Cout << "key: '" << reader.GetKeyText() << "' ( " << reader.GetOffset() << ", " << reader.GetCount() << ", " << reader.GetLength() << " )" << Endl;
    const size_t length1 = reader.GetLength();
    count += reader.GetCount();

    UNIT_ASSERT(reader.ReadNext());
//    Cout << "key: '" << reader.GetKeyText() << "' ( " << reader.GetOffset() << ", " << reader.GetCount() << ", " << reader.GetLength() << " )" << Endl;
    const size_t offset2 = reader.GetOffset();
    const size_t length2 = reader.GetLength();
    count += reader.GetCount();

    UNIT_ASSERT(reader.ReadNext());
//    Cout << "key: '" << reader.GetKeyText() << "' ( " << reader.GetOffset() << ", " << reader.GetCount() << ", " << reader.GetLength() << " )" << Endl;
    const size_t offset3 = reader.GetOffset();
    count += reader.GetCount();

    UNIT_ASSERT(!reader.ReadNext());
    UNIT_ASSERT_VALUES_EQUAL(count, 6000u);
    UNIT_ASSERT_VALUES_EQUAL(length1, offset2);
    UNIT_ASSERT_VALUES_EQUAL(length1 + length2, offset3);
}

