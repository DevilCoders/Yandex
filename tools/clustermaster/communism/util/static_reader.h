#pragma once

#include <library/cpp/archive/yarchive.h>

class TStaticReader: public TArchiveReader {
public:
    TStaticReader(const unsigned char* data, size_t dataSize);

    void GetStaticFile(const TString& key, TString& out) const;
    void GetStaticFile(const TString& key, IOutputStream& out) const;
};
