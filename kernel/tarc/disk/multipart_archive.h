#pragma once

#include "unpacker.h"

#include <kernel/multipart_archive/multipart.h>


template<NRTYArchive::IDataAccessor::TType OpenMode>
class TMultipartArchive : public TArchive<TMultipartArchivePtr> {
private:
    using TConstructContext = NRTYArchive::IArchivePart::TConstructContext;
public:
    TMultipartArchive(const IArchive::TCreationContext& ctx)
        : TArchive<TMultipartArchivePtr>(nullptr, ctx.ArchiveHeader)
    {
        NRTYArchive::TMultipartConfig config(ctx.MultipartConfig);
        config.ReadContextDataAccessType = OpenMode;
        ArcFile.Reset(TArchiveOwner::Create(ctx.ArchiveName, config, 0, true));
    }

    static IArchive::TFactory::TRegistrator<TMultipartArchive> Registrator;
};

typedef TMultipartArchive<NRTYArchive::IDataAccessor::TType::MEMORY_FROM_FILE>  TMemoryMultipartArchive;
typedef TMultipartArchive<NRTYArchive::IDataAccessor::TType::MEMORY_MAP>        TMapMultipartArchive;
typedef TMultipartArchive<NRTYArchive::IDataAccessor::TType::FILE>              TFileMultipartArchive;
typedef TMultipartArchive<NRTYArchive::IDataAccessor::TType::DIRECT_FILE>       TDirectFileMultipartArchive;
