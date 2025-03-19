#include "mega_wad_common.h"
#include "deduplicator.h"

namespace NDoom {

    struct TMegaWadInfoFactory {
        static THolder<TMegaWadInfo> Create(const TBlob& blob) {
            return MakeHolder<TMegaWadInfo>(LoadMegaWadInfo(blob));
        }
    };

    void TMegaWadCommon::Reset(const IChunkedBlobStorage* blobStorage, ui32 chunk) {
        Y_ENSURE(blobStorage->ChunkSize(chunk) >= 1);

        TBlob blob = blobStorage->Read(chunk, blobStorage->ChunkSize(chunk) - 1);
        Info_ = Deduplicator<TMegaWadInfoFactory>().GetOrCreate(blob);
        FillDocLumpTypeMap();
    }

    void EnableMegaWadInfoDeduplication() {
        Deduplicator<TMegaWadInfoFactory>().DeduplicationEnabled = true;
    }
}
