#pragma once

#include <kernel/indexer/directindex/extratext.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NRTYServer {
    class TZone;
}

void MakeSearchZones(const NRTYServer::TZone&, TVector<TUtf16String>* storage, NIndexerCore::TExtraTextZones*);
