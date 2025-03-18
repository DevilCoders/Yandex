#pragma once

#include <util/generic/string.h>

class IThreadPool;

struct TMD5Info {
    TString MD5;
    TString Error;

public:
    const TString& GetMD5OrError() const {
        return Error ? Error : MD5;
    }
};

void EraseFilenameFromMap(const TString& filename);
const TMD5Info& TryGetFileMD5FromMap(const TString& filename, IThreadPool& backgroundQueue);
const TMD5Info& GetFileMD5FromMap(const TString& filename);
