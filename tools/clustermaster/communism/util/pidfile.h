#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/system/file.h>

class TPidFile: TNonCopyable {
private:
    const TString Path;
    pid_t Pid;
    TFile File;
    bool DeleteOnExit;

public:
    TPidFile(const TString& path);
    ~TPidFile();

    void Update();
    void SetDeleteOnExit();
};
