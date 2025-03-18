#include "coded_blob.h"

#include <util/stream/buffer.h>
#include <util/stream/mem.h>

namespace NCodedBlob {
    static_assert(sizeof(CODED_BLOB_MAGIC) == NUtils::CODED_BLOB_MAGIC_SIZE + 1,
                  "expect sizeof(CODED_BLOB_MAGIC) == NUtils::CODED_BLOB_MAGIC_SIZE + 1");

    void TBasicBlob::Init(TBlob b) {
        TotalSize = b.Size();
        InitHeader(b);
        Data = NUtils::DoSkipBody(b);
        InitFooter(b);

        {
            TMemoryInput min(b.AsCharPtr(), b.Size());
            ::Load(&min, EntryCount);
        }
    }

    void TCodedBlob::InitHeader(TBlob& b) {
        NUtils::DoReadHeader(b, CODED_BLOB_MAGIC, CODED_BLOB_VERSION);
    }

    void TCodedBlob::InitFooter(TBlob& b) {
        {
            TMemoryInput min(b.AsCharPtr(), b.Size());
            Codec = NCodecs::ICodec::Restore(&min);
            b = b.SubBlob(b.Size() - min.Avail(), b.Size());
            ::Load(&min, EntryCount);
        }
    }

}
