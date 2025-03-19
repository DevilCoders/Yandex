#pragma once

#include "unpacker.h"

#include <util/memory/blob.h>

class TBlobArchive : public TArchive<TBlob> {
public:
    TBlobArchive(const IArchive::TCreationContext& ctx);

    TBlobArchive(const char* arcName, bool hasTextArchiveHeader = true);

    TBlobArchive(const TBlob& data, bool hasTextArchiveHeader = true);

    static IArchive::TFactory::TRegistrator<TBlobArchive> Registrator;
};

class TLockedBlobArchive : public TBlobArchive {
public:
    TLockedBlobArchive(const IArchive::TCreationContext& ctx);

    TLockedBlobArchive(const char* arcName, bool hasTextArchiveHeader = true);

    TLockedBlobArchive(const TBlob& data, bool hasTextArchiveHeader = true);

    static IArchive::TFactory::TRegistrator<TLockedBlobArchive> Registrator;
};
