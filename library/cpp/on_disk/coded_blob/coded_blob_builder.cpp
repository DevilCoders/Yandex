#include "coded_blob_builder.h"

#include <util/stream/buffer.h>

namespace NCodedBlob {
    void TCodedBlobBuilder::Init(IOutputStream* out, NCodecs::TCodecPtr trainedcodec) {
        Out = out;
        Codec = trainedcodec;
        Offset = 0;
        Count = 0;
        Y_VERIFY(!Codec || Codec->AlreadyTrained(), " bad codec");

        NUtils::DoWriteHeader(Out, CODED_BLOB_MAGIC, CODED_BLOB_VERSION);
    }

    void TCodedBlobBuilder::Finish() {
        TBufferOutput bout;
        NCodecs::ICodec::Store(&bout, Codec);
        ::Save(&bout, Count);
        NUtils::DoWriteWithFooter(Out, bout.Buffer().data(), bout.Buffer().size());
    }

}
