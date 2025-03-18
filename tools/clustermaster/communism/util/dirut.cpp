#include "dirut.h"

// When file does not exist
// Still fails when directory does not exist
TString RealPathFixed(const TString& path) {
    TString fileName = GetFileNameComponent(path.data());
    TString dirName = GetDirName(path);
    return RealPath(dirName) + '/' + fileName;
}
