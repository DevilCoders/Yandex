#pragma once

#include <library/cpp/deprecated/datafile/datafile.h>
#include <util/generic/string.h>
#include <util/system/defaults.h>


enum EPrechargeMode {
    // Concrete values are used for compatibility with old code.
    PCHM_Disable    = 0,
    PCHM_Force      = 1,
    PCHM_Auto       = 2,
    PCHM_Force_Lock = 3,    // PCHM_Force + lock the file in memory.
    PCHM_Complete,          // State after PCHM_Auto precharge.
};


class TPrechargableFile : public TDataFileBase {
public:
    TPrechargableFile(const char *fname, const int prechargeMode)
        : FileName(fname)
        , LoadMode(DLM_DEFAULT)
        , PrechargeMode(prechargeMode)
        , TotalRequests(0)
        , IsLockedInMemory(false)
    { }

    virtual ~TPrechargableFile() {
        MUnLock();
    }

    EDataLoadMode PrechargeModeToLoadMode(const int prechargeMode);

    void LoadFile(const char *fname, const EDataLoadMode loadMode);

    inline TString GetFileName() const {
        return FileName;
    }

    inline int GetPrechargeMode() const {
        return PrechargeMode;
    }

private:
    void MLock();
    void MUnLock();

protected:
    Y_FORCE_INLINE void REQUEST() const {
        TotalRequests++;
        if (Y_UNLIKELY(LoadMode == DLM_MMAP_AUTO_PRC && TotalRequests > 30)) {
            LoadMode = DLM_MMAP_PRC;
            TDataFileBase::Precharge();
        }
    }

private:
    TString                  FileName;
    mutable EDataLoadMode   LoadMode;

    // FIXME: Actually PrechargeMode should be a type of EPrechargeMode, but
    //        there is lots of code wich passes prechargeMode argument to the
    //        class methods as an integer. So it is int for backward
    //        compatibility.
    int                     PrechargeMode;

    mutable long            TotalRequests;  // Auto precharge after 30 requests.
    bool                    IsLockedInMemory;
};
