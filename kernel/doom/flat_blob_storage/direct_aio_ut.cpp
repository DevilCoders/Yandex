#include "direct_aio_flat_storage.h"
#include "mapped_flat_storage.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/generic/buffer.h>

Y_UNIT_TEST_SUITE(DirectAio) {
    TBlob RandomGarbage(size_t sz) {
        TBuffer buf;
        buf.Resize(sz);

        for (size_t i = 0; i < sz; i++) {
            buf.Data()[i] = RandomNumber<ui8>();
        }

        return TBlob::FromBuffer(buf);
    }

    void EnsureEqual(const TBlob& l, const TBlob& r) {
        UNIT_ASSERT_EQUAL(l.Size(), r.Size());
        UNIT_ASSERT(memcmp(l.Data(), r.Data(), l.Size()) == 0);
    }

    Y_UNIT_TEST(Read) {
        const TString fname = "file";
        const ui64 size = 40000;
        const ui64 requests = 200;
        auto data = RandomGarbage(size);
        {
            TUnbufferedFileOutput out(fname);
            out.Write(data.Data(), data.Size());
            out.Finish();
        }

        NDoom::TDirectAioFlatStorage mapped(TConstArrayRef<TString>{fname});
        TVector<ui32> parts;
        TVector<ui64> offsets;
        TVector<ui64> sizes;
        TVector<TBlob> blobs;

        for (size_t i = 0; i < requests; ++i) {
            parts.push_back(0);
            offsets.push_back(i * 100);
            sizes.push_back(300);
            blobs.push_back(TBlob());
        }
        mapped.Read(parts, offsets, sizes, blobs);

        for (size_t i = 0; i < requests; ++i) {
            EnsureEqual(TBlob::NoCopy(data.AsCharPtr() + offsets[i], sizes[i]), blobs[i]);
        }
    }
}
