#include "arcface.h"

#include <util/ysaveload.h>
#include <util/memory/blob.h>
#include <util/stream/zlib.h>
#include <util/string/builder.h>
#include <util/stream/mem.h>
#include <util/stream/buffer.h>
#include <util/stream/input.h>
#include <util/stream/output.h>


void WriteTextArchiveHeader(IOutputStream& out, TArchiveVersion archiveVersion) {
    out << ARCLABEL;
    Save(&out, archiveVersion);

    // don't know nice way to turn this verify into static assertion :(
    Y_VERIFY(ARCHIVE_FILE_HEADER_SIZE == strlen(ARCLABEL) + sizeof(archiveVersion),
            "%s", (TStringBuilder() << "expected header size: " << ARCHIVE_FILE_HEADER_SIZE
                << " actual header size: " << strlen(ARCLABEL) + sizeof(archiveVersion)).data());
}

void CheckTextArchiveHeader(IInputStream& in, TArchiveVersion& archiveVersion) {
    char buf[4];
    size_t cb = in.Load(buf, 4);
    if (cb != 4 || strncmp(buf, ARCLABEL, 4) != 0)
        throw TBadArcFormatException();
    Load(&in, archiveVersion);
}

ui64 WriteEmptyDoc(IOutputStream& out, const void* blob, size_t blobSize,  const void* packedtext, size_t textSize, ui32 docId) {
    TArchiveHeader hdr;
    hdr.DocId = docId;
    hdr.ExtLen = (ui32)blobSize;
    hdr.DocLen = ui32(sizeof(TArchiveHeader) + hdr.ExtLen + textSize);

    out.Write(&hdr, sizeof(hdr));
    out.Write(blob, blobSize);
    if (textSize)
        out.Write(packedtext, textSize);
    return (ui64)(sizeof(hdr) +  blobSize + textSize);
}

TBlob UnpackRawExtInfo(const TBlob& rawExtInfo, TArchiveVersion archiveVersion) {
    if (archiveVersion == ARCVERSION) {
        return rawExtInfo;
    } else if (archiveVersion == ARC_COMPRESSED_EXT_INFO) {
        TMemoryInput input(rawExtInfo.Data(), rawExtInfo.Size());
        TZLibDecompress decompressor((IInputStream*)&input);
        TBlob result = TBlob::FromStream(decompressor);
        return result;
    } else {
        ythrow yexception() << "Unknown archive version " << archiveVersion << " cannot unpack";
    }
}

TBlob PackRawExtInfo(const TBlob& rawExtInfo, TArchiveVersion archiveVersion) {
    if (archiveVersion == ARCVERSION) {
        return rawExtInfo;
    } else if (archiveVersion == ARC_COMPRESSED_EXT_INFO) {
        TBufferOutput bufferOutput;
        TZLibCompress compressor(&bufferOutput);
        compressor.Write(rawExtInfo.Data(), rawExtInfo.Size());
        compressor.Finish();
        return TBlob::FromBuffer(bufferOutput.Buffer());
    } else {
        ythrow yexception() << "Unknown archive version " << archiveVersion << " cannot pack";
    }
}
