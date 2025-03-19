#pragma once

#include <library/cpp/deprecated/mapped_file/mapped_file.h>

#include <util/system/defaults.h>
#include <util/system/filemap.h>

class TDocIdIterator {
private:
    TMappedFile Remap;
    ui32 Offset;

    bool Eof;
    ui32 Current;
    ui32 CurrentRemap;

    void ReadNextIt();
    void ReadNext();

public:
    TDocIdIterator(const char* index);
    bool IsEof() const;
    ui32 GetCurrent() const;
    ui32 GetCurrentRemap() const;
    void Next();
};
