#pragma once

#include "unpacker.h"

#include <util/generic/maybe.h>
#include <util/system/filemap.h>
#include <util/system/file.h>

class TFileArchive : public TArchive<TFile> {
public:
    TFileArchive(const IArchive::TCreationContext& ctx);

    TFileArchive(const char* arcName, bool reuse = true);

    static IArchive::TFactory::TRegistrator<TFileArchive> Registrator;
    static IArchive::TFactory::TRegistrator<TFileArchive> RegistratorNoReuse;
};
