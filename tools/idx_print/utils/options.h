#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <kernel/doom/info/index_format.h>

struct TIdxPrintOptions {
    TString IndexPath;
    bool Chunked = false;
    TString Query;
    bool ExactQuery = false;
    bool PrintHits = false;
    NDoom::EIndexFormat IndexFormat = NDoom::UnknownIndexFormat;
    bool YandexEncoded = false;
    bool HexEncodeBadChars = false;
    THashSet<ui32> DocIds;
    TVector<TString> WadPrinters;
};
