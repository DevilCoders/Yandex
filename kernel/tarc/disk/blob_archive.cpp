#include "blob_archive.h"

template<>
TBlob FromArchive<TBlob>(const TBlob& arc, ui64 docOffset, ui64 dataOffset, size_t length) {
    return TBlob::NoCopy((char*)arc.Data() + docOffset + dataOffset, length);
}

TBlobArchive::TBlobArchive(const IArchive::TCreationContext& ctx)
    : TBlobArchive(ctx.ArchiveName.data())
{}

TBlobArchive::TBlobArchive(const char* arcName, bool hasTextArchiveHeader)
    : TArchive<TBlob>(TBlob::FromFile(arcName), hasTextArchiveHeader)
{}

TBlobArchive::TBlobArchive(const TBlob& data, bool hasTextArchiveHeader)
    : TArchive<TBlob>(data, hasTextArchiveHeader)
{}

TLockedBlobArchive::TLockedBlobArchive(const IArchive::TCreationContext& ctx)
    : TLockedBlobArchive(ctx.ArchiveName.data())
{}

TLockedBlobArchive::TLockedBlobArchive(const char* arcName, bool hasTextArchiveHeader)
    : TBlobArchive(TBlob::LockedFromFile(arcName), hasTextArchiveHeader)
{}

TLockedBlobArchive::TLockedBlobArchive(const TBlob& data, bool hasTextArchiveHeader)
    : TBlobArchive(data, hasTextArchiveHeader)
{}

IArchive::TFactory::TRegistrator<TBlobArchive> TBlobArchive::Registrator({AT_FLAT, AOM_BLOB});
IArchive::TFactory::TRegistrator<TLockedBlobArchive> TLockedBlobArchive::Registrator({AT_FLAT, AOM_LOCKED_BLOB});
