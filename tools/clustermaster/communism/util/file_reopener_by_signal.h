#pragma once

#include <util/generic/string.h>
#include <util/system/file.h>

class TFileReopenerBySignal {
private:
    TString FileName;
    bool DupStdout;
    bool DupStderr;
    TFile File;
public:
    TFileReopenerBySignal(
        const TString& fileName,
        bool dupStdout = false,
        bool dupStderr = false);
    ~TFileReopenerBySignal();

    void Reopen();

    TFile& GetFile() {
        return File;
    }
};
