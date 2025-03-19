#include "arcdir.h"

TArchiveDir::TArchiveDir(const IArchiveDir::TCreationContext& ctx)
    : Base(nullptr)
    , Len(0)
{
    if (!ctx.UseMapping) {
        Blob = TBlob::FromFileContent(ctx.ArchiveDir);
    } else if (ctx.LockMemory) {
        Blob = TBlob::LockedFromFile(ctx.ArchiveDir);
    } else {
        Blob = TBlob::FromFile(ctx.ArchiveDir);
    }
    DoInit();
}

TArchiveDir::TArchiveDir(const char* dir)
    : Base(nullptr)
    , Len(0)
{
    Blob = TBlob::FromFileContent(dir);
    DoInit();
}

void TArchiveDir::DoInit() {
    Base = (const ui64*) Blob.Data();
    Len = Blob.Length() / sizeof(ui64);
}

TArchiveDir::TArchiveDir(const ui64 * const base, const ui32 size)
    : Base(base)
    , Len(size)
{}

void TArchiveDir::PreCharge() {
    if (!Blob.IsNull()) {
        Blob = Blob.DeepCopy();
        Base = (const ui64*) Blob.Data();
    }
}

ui32 TArchiveDir::Size() const {
    return Len;
}

bool TArchiveDir::Empty() const {
    return !Len;
}

bool TArchiveDir::HasDoc(ui32 docId) const {
    return (docId < Size() && operator[](docId) != FAIL_DOC_OFFSET);
}

ui64 TArchiveDir::operator[](ui32 docId) const {
    return Base[docId];
}

IArchiveDir::TFactory::TRegistrator<TArchiveDir> TArchiveDir::Registrator(AT_FLAT);

TMultipartDir::TMultipartDir(const IArchiveDir::TCreationContext& ctx)
    : Archive(TArchiveOwner::Find(ctx.ArchiveText))
{
    Y_VERIFY(Archive.Get());
    Len = Archive->GetDocsCount(true);
    DocsCount = Archive->GetDocsCount(false);
}

void TMultipartDir::PreCharge() {}

ui32 TMultipartDir::Size() const {
    return Len;
}

bool TMultipartDir::Empty() const {
    return DocsCount == 0;
}

bool TMultipartDir::HasDoc(ui32 docId) const {
    return docId < Size() && !Archive->IsRemoved(docId);
}

ui64 TMultipartDir::operator[](ui32 docid) const {
    return docid;
}

IArchiveDir::TFactory::TRegistrator<TMultipartDir> TMultipartDir::Registrator(AT_MULTIPART);
