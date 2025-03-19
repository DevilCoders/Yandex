#include "archive_buffer.h"

// We assume that we have only one writer

class TMemoryArchiveBuffer::TDocGuard : public IMemoryArchiveDocGuard {
private:
    const TMemoryArchiveBuffer::TDocPtr Doc;

public:
    TDocGuard(const TMemoryArchiveBuffer::TDocPtr doc)
        : Doc(doc)
    {
    }
};

TMemoryArchiveBuffer::TMemoryArchiveBuffer(ui32 maxDocs)
    : IMemoryArchive(maxDocs)
    , DocBuffers(maxDocs)
{ }

TMemoryArchiveBuffer::~TMemoryArchiveBuffer() {
}

void TMemoryArchiveBuffer::DoWrite(const void* data, size_t size) {
    Y_VERIFY(DocBuffers[CurrentDocId]);
    TBuffer& buffer = *DocBuffers[CurrentDocId];
    if (Y_LIKELY(buffer.Avail() < size))
        buffer.Reserve(buffer.Size() + size);
    buffer.Append((const char*)data, size);
}

void TMemoryArchiveBuffer::AddDoc(ui32 docId) {
    CurrentDocId = docId;
    SetDoc(docId, MakeIntrusive<TDoc>());
}

void TMemoryArchiveBuffer::EraseDoc(ui32 docId) {
    SetDoc(docId, nullptr);
}

bool TMemoryArchiveBuffer::HasDoc(ui32 docId) const {
    return DocBuffers[docId].Get();
}

IMemoryArchiveDocGuardPtr TMemoryArchiveBuffer::GuardDoc(ui32 docId) const {
    auto doc = GetDoc(docId);
    if (!doc) {
        throw yexception() << "DocId " << docId << " does not exist in MemoryArchive";
    }

    return MakeHolder<TDocGuard>(doc);
}

TBlob TMemoryArchiveBuffer::GetDocBlob(ui32 docId) const {
    auto doc = DocBuffers[docId].Get();
    if (!doc) {
        return TBlob();
    }

    return TBlob::NoCopy(doc->Data(), doc->Size());
}

TMemoryArchiveBuffer::TDocPtr TMemoryArchiveBuffer::GetDoc(ui32 docId) const {
    TReadGuard guard(Locks[docId % Locks.size()]);
    return DocBuffers[docId].Get();
}

void TMemoryArchiveBuffer::SetDoc(ui32 docId, TDocPtr doc) {
    TWriteGuard guard(Locks[docId % Locks.size()]);
    DocBuffers[docId] = doc;
}

void TMemoryArchiveBuffer::GetExtInfoAndDocText(ui32 docId, TBlob& extInfoBlob, TBlob& docTextBlob, bool getExtInfo, bool getDocText) const {
    TUnpackDocCtx ctx;
    TBlob docBlob = GetDocBlob(docId);
    ctx.Set(docBlob, 0);
    TArchive<TBlob> ArchiveText(docBlob, false);
    IMemoryArchive::GetExtInfoAndDocText(ArchiveText, ctx, extInfoBlob, docTextBlob, getExtInfo, getDocText);
}
