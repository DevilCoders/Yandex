#include "coded_blob_array.h"

namespace NCodedBlob {
    static_assert(sizeof(CODED_BLOB_ARRAY_MAGIC) == NUtils::CODED_BLOB_MAGIC_SIZE + 1,
                  "expect sizeof(CODED_BLOB_ARRAY_MAGIC) == NUtils::CODED_BLOB_MAGIC_SIZE + 1");

    void TCodedBlobArray::Init(TBlob b) {
        TotalSize = b.Size();
        NUtils::DoReadHeader(b, CODED_BLOB_ARRAY_MAGIC, CODED_BLOB_ARRAY_VERSION);
        Data.Init(NUtils::DoSkipBody(b));

        {
            TMemoryInput min(b.AsCharPtr(), b.Size());
            Offsets.Load(&min);
            IndexSize = b.Size() - min.Avail();
        }
    }

}
