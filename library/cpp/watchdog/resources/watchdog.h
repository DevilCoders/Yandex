#pragma once

#include <library/cpp/watchdog/lib/handle.h>
#include <library/cpp/watchdog/lib/interface.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>

class TResourcesWatchDogHandle: public IWatchDogHandle {
public:
    TResourcesWatchDogHandle(size_t maxVirtualMemorySize, int maxFileDescriptors, TDuration delay);

    static IWatchDog* Create(size_t maxVirtualMemorySize, int maxFileDescriptors, TDuration delay);

    void Check(TInstant timeNow) override;

private:
    size_t MaxVirtualMemorySize; // Maximum allowed virtual memory size in bytes
    int MaxFileDescriptors;      // Maximum number of opened file descriptors
    const TInstant Start;        // When the check for resources will start

#if defined(_linux_)
    TString statFile;    // /proc/[pid]/stat
    TString fdDirectory; // /proc/[pid]/fd
#endif
#if defined(_linux_) || defined(_freebsd_)
    pid_t Pid;
#endif
    TVector<char> Buffer;
};
