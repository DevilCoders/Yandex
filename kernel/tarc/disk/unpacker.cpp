#include "unpacker.h"

#include <library/cpp/charset/wide.h>

#include <util/generic/buffer.h>
#include <util/stream/zlib.h>
#include <util/stream/mem.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

void TUnpackDocCtx::Set(const TBlob& arcFile, ui64 offset) {
    if (offset != FAIL_DOC_OFFSET && (ui64)arcFile.Size() >= offset + sizeof(TArchiveHeader)) {
        Offset = offset;
        memcpy(&Header, (char*)arcFile.Data() + offset, sizeof(TArchiveHeader));
    } else {
        Offset = FAIL_DOC_OFFSET;
        Header = TArchiveHeader();
    }
}

void TUnpackDocCtx::Set(const TMemoryMap& arcFile, ui64 offset) {
    if (offset != FAIL_DOC_OFFSET && (ui64)arcFile.Length() >= offset + sizeof(TArchiveHeader)) {
        Offset = offset;
        TFileMap localMap(arcFile);
        if (!localMap.IsOpen())
            ythrow yexception() << "memory map not open";

        localMap.Map(Offset, sizeof(TArchiveHeader));

        if (!localMap.Ptr())
            ythrow yexception() << "can not map(" <<  Offset << ", " <<  (unsigned long)sizeof(TArchiveHeader) << ")";
        Y_ASSERT(localMap.MappedSize() == sizeof(TArchiveHeader));
        memcpy(&Header, localMap.Ptr(), localMap.MappedSize());
    } else {
        Offset = FAIL_DOC_OFFSET;
        Header = TArchiveHeader();
    }
}

void TUnpackDocCtx::Set(const TFile& arcFile, ui64 offset) {
    if (offset != FAIL_DOC_OFFSET) {
        Y_VERIFY((ui64)arcFile.GetLength() >= offset + sizeof(TArchiveHeader), "incorrect archive offset %" PRIu64, offset);
        Offset = offset;
        size_t length = sizeof(TArchiveHeader);
        arcFile.Pload(&Header, length, offset);
    } else {
        Offset = FAIL_DOC_OFFSET;
        Header = TArchiveHeader();
    }
}

void TUnpackDocCtx::Set(const TMultipartArchivePtr& arcFile, ui64 docId) {
    TBlob data = arcFile->GetDocument(docId);
    if (!data.Empty()) {
        Offset = docId;
        memcpy(&Header, data.Data(), sizeof(TArchiveHeader));
        Header.DocId = docId; // Cause only FAT knows correct docId.
    } else {
        Offset = FAIL_DOC_OFFSET;
        Header = TArchiveHeader();
    }
}
