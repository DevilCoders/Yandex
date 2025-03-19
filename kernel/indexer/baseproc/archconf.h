#pragma once

#include <util/generic/string.h>

struct TArchiveProcessorConfig {
    TString Archive;
    TString DocProperties;
    TString PassageProperties;
    size_t MaxDocSize;
    bool   UseArchive;
    bool   UseFullArchive;
    bool   SaveHeader;
    bool   CompressExtInfo;

    TArchiveProcessorConfig()
        : MaxDocSize(0xffffffff)
        , UseArchive(true)
        , SaveHeader(true)
        , CompressExtInfo(false)
    {}
};
