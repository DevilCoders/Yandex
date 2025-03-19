#pragma once

#include <util/system/defaults.h>

#include <kernel/tarc/disk/searcharc.h>

#include "id2string.h"

class TArchiveUrlExtractor : public IId2String
{
private:
    TSearchArchive Archive;
    TSearchFullArchive FullArchive;

public:
    TArchiveUrlExtractor(const TString& input);
    TString GetString(ui32 docId) override;
};
