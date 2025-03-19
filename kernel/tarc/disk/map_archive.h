#pragma once

#include "unpacker.h"

#include <util/memory/blob.h>

class TMapArchive : public TArchive<TMemoryMap> {
public:
    TMapArchive(const IArchive::TCreationContext& ctx);

    TMapArchive(const char* arcName);

    static IArchive::TFactory::TRegistrator<TMapArchive> Registrator;
};
