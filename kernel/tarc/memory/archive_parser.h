#pragma once

#include "archive_buffer.h" //for TMemoryArchiveBuffer

class TArchiveParser : private TMemoryArchiveBuffer {
public:
    TArchiveParser();
    void Parse(const TString& archiveString, TBlob *extInfo, TBlob *docText);
};
