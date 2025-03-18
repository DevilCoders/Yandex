#pragma once

#include <util/system/thread.h>

class TDiskspaceNotifier {
public:
    struct TParams
    {
        unsigned Delay;
        TString MountPoint;

        TParams()
        :   Delay(30)
        ,   MountPoint("/place") //yandex-search common mount point
        {
        }

        TParams& SetDelay(unsigned delay)
        {
            Delay = delay;
            return *this;
        }
        TParams& SetMountPoint(const TString& mountPoint)
        {
            MountPoint = mountPoint;
            return *this;
        }
    };

    TDiskspaceNotifier(const TParams &params);

private:
    const TParams Params;
    TThread Thread;
};
