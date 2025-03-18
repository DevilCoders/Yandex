#include "coded_blob_array_builder.h"

#include <util/stream/buffer.h>

namespace NCodedBlob {
    void TCodedBlobArrayBuilder::Init(IOutputStream* out, NCodecs::TCodecPtr trainedcodec) {
        Out = out;
        NUtils::DoWriteHeader(Out, CODED_BLOB_ARRAY_MAGIC, CODED_BLOB_ARRAY_VERSION);
        DataBuilder.Init(out, trainedcodec);
    }

    void TCodedBlobArrayBuilder::Finish() {
        DataBuilder.Finish();
        {
            TOffsetsArray arr;
            arr.Learn(Offsets.empty() ? 1 : Offsets.back() + 1, Offsets.size());
            for (ui64 i = 0, sz = Offsets.size(); i < sz; ++i) {
                arr.Add(Offsets[i]);
            }
            arr.Finish();

            TBufferOutput bout;
            arr.Save(&bout);
            NUtils::DoWriteWithFooter(Out, bout.Buffer().data(), bout.Buffer().size());
        }
    }

}
