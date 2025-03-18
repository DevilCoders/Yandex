#pragma once

#include "hw_resources.h"

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

/* Resource usage in percent [0-100] for a process group */

struct resUsageStat_st {
    double cpuMax;
    double cpuMaxDelta;
    double cpuAvg;
    double cpuAvgDelta;
    double memMax;
    double memMaxDelta;
    double memAvg;
    double memAvgDelta;
    double ioMax;
    double ioMaxDelta;
    double ioAvg;
    double ioAvgDelta;
    ui32 updateCounter;
    TSimpleSharedPtr<TMap<TString, double> > sharedAreas;
};


inline const resUsageStat_st GetDefaultResUsage() {
    machineInfo machInfo;
    THWResources res;
    res.getHWInfo(&machInfo);

    resUsageStat_st defaultResUsage = {
        1.0 / machInfo.nCPU, // cpuMax
        0.01,                // cpuMaxDelta
        1.0 / machInfo.nCPU, // cpuAvg (approx. one core)
        0.01,                // cpuAvgDelta
        0.01,                // memMax
        0.01,                // memMaxDelta
        0.01,                // memAvg
        0.01,                // memAvgDelta
        0.05,                // ioMax
        0.01,                // ioMaxDelta
        0.01,                // ioAvg
        0.01,                // ioAvgDelta
        0,                   // updateCounter
        nullptr
    };
    return defaultResUsage;
}

const resUsageStat_st defaultResUsage = GetDefaultResUsage();
