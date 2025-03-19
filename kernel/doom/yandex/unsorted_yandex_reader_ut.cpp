#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

#include <kernel/doom/yandex/unsorted_yandex_reader.h>
#include <kernel/doom/hits/superlong_hit.h>
#include <util/stream/file.h>

using namespace NDoom;

extern "C" {
    extern const unsigned char UnsortedIndex[];
    extern const ui32 UnsortedIndexSize;
};

Y_UNIT_TEST_SUITE(UnsortedYandexReader) {
    Y_UNIT_TEST(ReadAll) {
        TArchiveReader archive(TBlob::NoCopy(UnsortedIndex, UnsortedIndexSize));
        {
            TBlob inv = archive.ObjectBlobByKey("/unsorted_index.inv");
            TOFStream("unsorted_index.inv").Write(inv.AsCharPtr(), inv.Size());
            TBlob key = archive.ObjectBlobByKey("/unsorted_index.key");
            TOFStream("unsorted_index.key").Write(key.AsCharPtr(), key.Size());
        }

        TUnsortedYandexReader<NDoom::TSuperlongHit> reader("unsorted_index.");
        size_t hitCount = 0;
        TStringBuf key;
        while (reader.ReadKey(&key)) {
            NDoom::TSuperlongHit hit;
            while (reader.ReadHit(&hit)) {
                ++hitCount;
            }
        }
    }
}
