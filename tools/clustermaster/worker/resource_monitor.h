#pragma once
/*
 * Resource monitoring module.
 * Will log general resource usage to the file.
 * Log file dir is determined from environment variable RESMON_PATH
 *
 * Also, monitoring of resources usage by process group is supported.
 *
 */

#include "hw_resources.h"

#include <tools/clustermaster/common/resource_monitor_resources.h>

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/system/mutex.h>
#include <util/system/sigset.h>
#include <util/system/thread.h>

const struct resUsageStat_st zeroResUsage = {
                                          0.00,           // cpuMax
                                          0.00,           // cpuMaxDelta
                                          0.00,           // cpuAvg
                                          0.00,           // cpuAvgDelta
                                          0.00,           // memMax
                                          0.00,           // memMaxDelta
                                          0.00,           // memAvg
                                          0.00,           // memAvgDelta
                                          0.00,           // ioMax
                                          0.00,           // ioMaxDelta
                                          0.00,           // ioAvg
                                          0.00,           // ioAvgDelta
                                          0,              // updateCounter
                                          nullptr
};

const struct resUsageStat_st smallResUsage = {
                                          0.01,           // cpuMax
                                          0.01,           // cpuMaxDelta
                                          0.01,           // cpuAvg
                                          0.01,           // cpuAvgDelta
                                          0.01,           // memMax
                                          0.01,           // memMaxDelta
                                          0.01,           // memAvg
                                          0.01,           // memAvgDelta
                                          0.00,           // ioMax
                                          0.00,           // ioMaxDelta
                                          0.00,           // ioAvg
                                          0.00,           // ioAvgDelta
                                          0,              // updateCounter
                                          nullptr
};


/* Global REsources Statistics */
struct globalStat
{
    double cpuUse;            /* Actual information, update once in Delay period */
    double cpuAvg1min;        /* CPU average for 1 minute */
    ui32 memPagesUse;         /* Mem using (in hw pages)  */
    long double IO_Busy;
    long double IO_MBpS;
};

struct TWorkerGlobals;

class TResourceMonitor: public TThread, public THWResources {
public:
    typedef TMap<pid_t, struct resUsageStat_st* > TGroupsStats;
    virtual int startGroupMonitoring(pid_t pgid);
    virtual int getGroupStatistics(pid_t pgid, struct resUsageStat_st * groupStat);
    virtual int rmProcessGroup(pid_t pgid, struct resUsageStat_st* groupStat = nullptr);

    TResourceMonitor(const TWorkerGlobals* workerGlobals, bool forTest);
    virtual ~TResourceMonitor();

public:
    void Run() noexcept;

private:
    static const ui32 MIN_RSS_SIZE = 102400;
    static const ui32 MIN_SHAREDMEM_SIZE = 51200;

    typedef TGroupsStats::iterator groupIt;
    /* delay between stat update */
    bool TimeToDie;
    int Delay;

    TMutex Mutex;

    FILE *logStat;

    THolder<TGroupsStats> monitoredGroups;
    TGroupsStats finishedGroups;

    struct globalStat curGlobalStat;

    static void* ThreadProc(void* param) noexcept;

    void initIOMon();
    int updateCPULoad();
    int updateMemUsage();

    int determineBerkanavtDevID();

    int gatherActiveGroupsStat();
    int updateActiveGroupStat(TVector<KInfoCommonPtr>& procGroupInfo,
                              struct resUsageStat_st* groupResStat);
    void printProcStat(KInfoCommonPtr const& procStat);
    void printProcStat(TVector<KInfoCommonPtr> const& procGroupInfo);
};
