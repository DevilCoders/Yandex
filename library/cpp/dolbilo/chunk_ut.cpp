#include "chunk.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/system/tempfile.h>

#define CDATA "./chunkedio"

Y_UNIT_TEST_SUITE(TestBinaryChunks) {
    static const char test_data[] = "87s6cfbsudg cuisg s igasidftasiy tfrcua6s";

    Y_UNIT_TEST(TestBinaryChunkedIo) {
        TTempFile tmpFile(CDATA);
        TString tmp;

        {
            TUnbufferedFileOutput fo(CDATA);
            TBinaryChunkedOutput co(&fo);

            for (size_t i = 0; i < sizeof(test_data); ++i) {
                co.Write(test_data, i);
                tmp.append(test_data, i);
            }

            co.Finish();
            fo.Finish();
        }

        {
            TUnbufferedFileInput fi(CDATA);
            TBinaryChunkedInput ci(&fi);
            TString r;

            char buf[11];
            size_t readed = 0;

            do {
                readed = ci.Read(buf, sizeof(buf));
                r.append(buf, readed);
            } while (readed);

            UNIT_ASSERT_EQUAL(r, tmp);
        }
    }
}
