#include "prechargable_file.h"

#include <util/system/mlock.h>

EDataLoadMode TPrechargableFile::PrechargeModeToLoadMode(const int prechargeMode) {
    PrechargeMode = prechargeMode;
    LoadMode = (PCHM_Disable == prechargeMode ? DLM_MMAP :
                PCHM_Auto    == prechargeMode ? DLM_MMAP_AUTO_PRC :
                DLM_MMAP_PRC);
    return LoadMode;
}

void TPrechargableFile::LoadFile(const char *fname, const EDataLoadMode loadMode) {
    FileName = fname;
    LoadMode = loadMode;
    TDataFileBase::DoLoad(FileName.data(), LoadMode);
    MLock();
}

void TPrechargableFile::MLock() {
    if (PCHM_Force_Lock == PrechargeMode) {
        LockMemory((void*)TDataFileBase::Start, TDataFileBase::Length);
        IsLockedInMemory = true;
    }
}

void TPrechargableFile::MUnLock() {
    if (IsLockedInMemory) {
        UnlockMemory((void*)TDataFileBase::Start, TDataFileBase::Length);
        IsLockedInMemory = false;
    }
}
