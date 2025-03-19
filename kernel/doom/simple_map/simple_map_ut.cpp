#include "simple_map_reader.h"
#include "simple_map_writer.h"

#include <kernel/doom/hits/counts_hit.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TSimpleMapReaderWriterTest) {

    TCountsHit Hit(ui32 doc, ui32 streamType, ui32 value) {
        return TCountsHit(doc, NDoom::EStreamType(streamType), value);
    }

    using THit = TCountsHit;
    using TIndexMap = TMap<TString, TVector<THit>>;

    Y_UNIT_TEST(TestWriteAndRead) {
        TIndexMap index;
        index["a"] = {Hit(0, 0, 1), Hit(0, 0, 1), Hit(25, 20, 1)};
        index["ab"] = {Hit(1, 2, 4), Hit(4, 3, 1)};
        index["z"] = {Hit(100, 2, 100), Hit(100, 3, 120), Hit(101, 2, 100)};

        /* Write-test */
        TIndexMap writtenIndex;
        TSimpleMapWriter<THit> writer(writtenIndex);
        for (const auto& keyInfo : index) {
            for (THit hit: keyInfo.second) {
                writer.WriteHit(hit);
            }
            writer.WriteKey(keyInfo.first);
        }
        writer.Finish();

        UNIT_ASSERT(index == writtenIndex);

        /* Read-test */
        TSimpleMapReader<THit> reader(writtenIndex);
        TStringBuf key;
        TIndexMap::const_iterator currentKey = index.begin();
        while (reader.ReadKey(&key)) {
            UNIT_ASSERT_VALUES_EQUAL(currentKey->first, key);
            for (const THit hit : currentKey->second) {
                THit readHit;
                UNIT_ASSERT(reader.ReadHit(&readHit));
                UNIT_ASSERT_VALUES_EQUAL(hit.ToSuperLong(), readHit.ToSuperLong());
            }
            THit readHit;
            UNIT_ASSERT(!reader.ReadHit(&readHit));
            ++currentKey;
        }
        UNIT_ASSERT(currentKey == index.end());

        /* Seek test */
        reader.Seek("ac");
        TStringBuf readKey;
        UNIT_ASSERT(reader.ReadKey(&readKey));
        UNIT_ASSERT_VALUES_EQUAL("z", readKey);

        reader.Seek("za");
        UNIT_ASSERT(!reader.ReadKey(&readKey));
    }

}
