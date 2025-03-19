#include "file_archive.h"

template<>
TBlob FromArchive<TFile>(const TFile& arc, ui64 docOffset, ui64 dataOffset, size_t length) {
    return TBlob::FromFileContent(const_cast<TFile&>(arc), docOffset + dataOffset, length);
}

TFileArchive::TFileArchive(const IArchive::TCreationContext& ctx)
    : TFileArchive(ctx.ArchiveName.data(), ctx.IsFileReuse)
{}

TFileArchive::TFileArchive(const char* arcName, bool reuse)
    : TArchive<TFile>(TFile(
        arcName,
        OpenExisting |
        RdOnly |
        (reuse ? EOpenMode() : NoReuse)
    ))
{}

IArchive::TFactory::TRegistrator<TFileArchive> TFileArchive::Registrator({AT_FLAT, AOM_FILE});
IArchive::TFactory::TRegistrator<TFileArchive> TFileArchive::RegistratorNoReuse({AT_FLAT, AOM_FILE_NO_REUSE});
