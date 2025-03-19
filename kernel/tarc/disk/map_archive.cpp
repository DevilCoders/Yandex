#include "map_archive.h"

template<>
TBlob FromArchive<TMemoryMap>(const TMemoryMap& arc, ui64 docOffset, ui64 dataOffset, size_t length) {
    return TBlob::FromMemoryMap(arc, docOffset + dataOffset, length);
}

TMapArchive::TMapArchive(const IArchive::TCreationContext& ctx)
    : TMapArchive(ctx.ArchiveName.data())
{}

TMapArchive::TMapArchive(const char* arcName)
    : TArchive<TMemoryMap>(TMemoryMap(arcName))
{}

IArchive::TFactory::TRegistrator<TMapArchive> TMapArchive::Registrator({AT_FLAT, AOM_MAP});
