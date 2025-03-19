#pragma once

#include <library/cpp/archive/yarchive.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>

class TDictArchive {
public:
    inline TDictArchive(const unsigned char* data, size_t len) {
        TArchiveReader reader(TBlob::NoCopy(data, len));
        TString key = reader.KeyByIndex(0);
        Data = reader.ObjectBlobByKey(key);
    }
    TBlob GetBlob() const {
        return Data;
    }

private:
    TBlob Data;
};
