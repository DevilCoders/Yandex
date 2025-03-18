#include "resource_monitor.h"

#include "log.h"
#include "worker.h"

#include <tools/clustermaster/common/thread_util.h>

int TResourceMonitor::updateCPULoad()
{
    int err = getCPULoad(&curGlobalStat.cpuAvg1min);

    err |= getCPUUse(&curGlobalStat.cpuUse);

    return err;
}

int TResourceMonitor::updateMemUsage()
{
    curGlobalStat.memPagesUse = hwInfo.maxMemPages - getFreeMem();

    return 0;
}

int TResourceMonitor::startGroupMonitoring(pid_t pgid)
{
    groupIt it = monitoredGroups->find(pgid);
    if (it != monitoredGroups->end()) {
        ERRORLOG("RESMON: Group monitoring is already started for " << pgid <<
                "pid; Total monitored " << (int)monitoredGroups->size() << " groups.");
        return 1;
    }

    it = finishedGroups.find(pgid);
    if (it != finishedGroups.end()) {
        ERRORLOG("RESMON: There is stat info for " << pgid <<
                "pid; Assume that it is spoiled. Total monitored " << (int)monitoredGroups->size() << " groups.");

        rmProcessGroup(it->first);
    }
    struct resUsageStat_st* emptyStatistics = new struct resUsageStat_st(zeroResUsage);
    emptyStatistics->sharedAreas = new TMap<TString, double>;

    TGuard<TMutex> guard(Mutex);
    (*monitoredGroups)[pgid] = emptyStatistics;

     return 0;
}

int TResourceMonitor::getGroupStatistics(pid_t pgid, struct resUsageStat_st * groupStat)
{
    TGuard<TMutex> guard(Mutex);

    groupIt it = finishedGroups.find(pgid);
    if (it == finishedGroups.end()) {
        it = monitoredGroups->find(pgid);
        if (it == monitoredGroups->end()) {
            ERRORLOG("RESMON: There is no stat info for " << pgid <<
                    " pid; Total monitored " << (int)monitoredGroups->size() << " groups.");
            return 1;
        }
    }

    /*
     * Returned results are all in percent [0-MAX_RANGE]
     */
    groupStat->cpuAvg = it->second->cpuAvg;
    groupStat->cpuMax = it->second->cpuMax;
    groupStat->memAvg = it->second->memAvg;
    groupStat->memMax = it->second->memMax;
    groupStat->ioAvg = it->second->ioAvg;
    groupStat->ioMax = it->second->ioMax;
    groupStat->sharedAreas = it->second->sharedAreas;

    return 0;
}

int TResourceMonitor::rmProcessGroup(pid_t pgid, struct resUsageStat_st* groupStat)
{
    TGuard<TMutex> guard(Mutex);

    groupIt it = finishedGroups.find(pgid);
    if (it == finishedGroups.end()) {
        it = monitoredGroups->find(pgid);
        if (it == monitoredGroups->end()) {
            ERRORLOG("RESMON: Info is absent for " << pgid <<
                    " pid. Total monitored " << (int)monitoredGroups->size() << " groups.");
            return 1;
        }

        if (groupStat) {
            *groupStat = *it->second;
        }

        delete it->second;
        monitoredGroups->erase(it);
    } else {
        if (groupStat) {
            *groupStat = *it->second;
        }

        delete it->second;
        finishedGroups.erase(it);
    }

    /*
     * ToDo: return stat info
     */
    return 0;
}

void TResourceMonitor::printProcStat(KInfoCommonPtr const& procStat)
{
    if ((procStat->ki_cpuavg < 1.00) && (100*procStat->ki_rssize / hwInfo.maxMemPages < 1))
        return;

    fprintf(logStat, "\t## pid\t%d:\tcpu=%.02f\tmem=%ld\n",
            procStat->ki_pid, procStat->ki_cpuavg, procStat->ki_rssize);

    for (TMap<TString, ui32>::iterator it = procStat->SharedMemAreas->begin();
                it != procStat->SharedMemAreas->end(); ++it) {
        fprintf (logStat, "\t\t%s=%d\n", it->first.data(), (it->second));
    }
}

void TResourceMonitor::printProcStat(TVector<KInfoCommonPtr> const& procStat)
{
    if (procStat.size()) {
        fprintf(logStat, "### pgid %d :\n", procStat[0]->ki_pgid);
        for (TVector<KInfoCommonPtr>::const_iterator it = procStat.begin();
                it != procStat.end(); ++it) {
            printProcStat(*it);
        }
    }
    fprintf(logStat, "\n");
}

int TResourceMonitor::updateActiveGroupStat (TVector<KInfoCommonPtr>& procGroupInfo,
                         struct resUsageStat_st* groupResStat)
{
    /* Update mem & cpu stat */
    double cpuTotalWCPU = 0;
    for (size_t i = 0; i < procGroupInfo.size(); ++i) {
        cpuTotalWCPU += procGroupInfo[i]->ki_cpuavg;
    }

    /* Count shared and non-shared pages number */
    i64 rssPagesCount = 0;
    TMap<TString, ui32> AllSharedMemAreas;
    for (ui16 i = 0; i < procGroupInfo.size(); ++i) {
        ui32 sharedPagesCount = 0;
        rssPagesCount += procGroupInfo[i]->ki_rssize;

        for (TMap<TString, ui32>::iterator it = procGroupInfo[i]->SharedMemAreas->begin();
                it != procGroupInfo[i]->SharedMemAreas->end(); ++it) {
            sharedPagesCount += it->second;
            AllSharedMemAreas[it->first] = Max (AllSharedMemAreas[it->first], it->second);
        }
    }


    for (TMap<TString, ui32>::iterator it = AllSharedMemAreas.begin();
            it != AllSharedMemAreas.end(); ++it) {
        (*groupResStat->sharedAreas)[it->first] = Max ((*groupResStat->sharedAreas)[it->first],
                                                        (double)it->second/hwInfo.maxMemPages);
    }

    /* Выведем статистику для группы. I/O еще нет ABS_MEM=(4*usedPageCount/1024))Mb; */
    double memUsePercent = double(rssPagesCount) / hwInfo.maxMemPages;
    if ((memUsePercent < 0.00) || (memUsePercent > 1.00) || cpuTotalWCPU > 1.00) {
        fprintf(logStat, "# SORROW: MemUse = %02f, CpuUse = %02f -->\n", memUsePercent, cpuTotalWCPU);

        printProcStat(procGroupInfo);
        return 1;
    }

    cpuTotalWCPU = Min (0.99, cpuTotalWCPU);

//    fprintf(logStat, "pgid\t%d:\t%d\t%d\t0\t0\n",
//            procGroupInfo[0]->ki_pgid, (int)(cpuTotalWCPU * 100), (int)(memUsePercent * 100));

    if ((memUsePercent > groupResStat->memMax) || (cpuTotalWCPU > groupResStat->cpuMax)) {
        fprintf(logStat, "# Maximum refresh:\n");
        printProcStat(procGroupInfo);
    }

    if (groupResStat->updateCounter) {
        groupResStat->memMax = Max(groupResStat->memMax, memUsePercent);
        groupResStat->cpuMax = Max(groupResStat->cpuMax, cpuTotalWCPU);
    } else { /* There are no info actually */
        groupResStat->memMax = memUsePercent;
        groupResStat->cpuMax = cpuTotalWCPU;
    }


    groupResStat->memAvg = (double)(groupResStat->memAvg * groupResStat->updateCounter + memUsePercent) /
            (groupResStat->updateCounter + 1);
    groupResStat->cpuAvg = (double)(groupResStat->cpuAvg * groupResStat->updateCounter + cpuTotalWCPU) /
            (groupResStat->updateCounter + 1);

    groupResStat->updateCounter++;

    return 0;
}

int TResourceMonitor::gatherActiveGroupsStat()
{
    TGuard<TMutex> guard(Mutex);

    if (monitoredGroups->size()) {
        /* Init */
        TMap<pid_t, TVector<KInfoCommonPtr> > procGroupInfos;
        TVector<KInfoCommonPtr> emptyStat;

        for (groupIt it = monitoredGroups->begin();
            it != monitoredGroups->end(); ++it) {
            procGroupInfos[it->first] = emptyStat;
        }

        int err = getProcessGroupInfo(&procGroupInfos);

        if (!err) {
            for (TMap<pid_t, TVector<KInfoCommonPtr> >::iterator it = procGroupInfos.begin();
                it != procGroupInfos.end(); ++it) {
                if (it->second.size()) {
                    struct resUsageStat_st* groupResStat = monitoredGroups->find(it->first)->second;
                    updateActiveGroupStat(it->second, groupResStat);
                    it->second.clear();
                } else { /* there are no processes in the group. Move it to finished */
                    pid_t gpid = it->first;
                    groupIt git = monitoredGroups->find(gpid);
                    finishedGroups[gpid] = git->second;
                    monitoredGroups->erase(git);
                }
            }
        }
    }

    return 0;
}

void* TResourceMonitor::ThreadProc(void* param) noexcept
{
    TResourceMonitor *resMon = reinterpret_cast<TResourceMonitor*>(param);
    SetCurrentThreadName("resource monitor");
    resMon->Run();
    return nullptr;
}

void TResourceMonitor::Run() noexcept
{
    sigset_t newMask, oldMask;
    SigFillSet(&newMask);
    SigProcMask(SIG_BLOCK, &newMask, &oldMask);

    ui32 count = 0;
    while (!TimeToDie) {
        count++;

        updateCPULoad();
        if (count == 5) {
            count = 0;
            updateMemUsage();
            updateIOStat(&curGlobalStat.IO_Busy, &curGlobalStat.IO_MBpS);

            ui32 cpuUse      = ceil(100 * curGlobalStat.cpuUse);
            ui32 cpuLoad1min = ceil(100 * curGlobalStat.cpuAvg1min);
            ui32 memUsage    = ceil(double(100 * curGlobalStat.memPagesUse)/hwInfo.maxMemPages);
            ui32 ioUsage     = ceil(curGlobalStat.IO_Busy);
            ui32 ioLoad      = ceil(curGlobalStat.IO_MBpS); /* Suppose maximum data rate at 100Mb/s */

            fprintf(logStat, "%ld\t%u\t%u\t%u\t%u\t%u\n",
                    TInstant::Now().TimeVal().tv_sec,
                    cpuUse,
                    cpuLoad1min,
                    memUsage,
                    ioUsage, ioLoad);
        }

        gatherActiveGroupsStat();
        sleep(Delay);
    }
}

TResourceMonitor::TResourceMonitor(const TWorkerGlobals* workerGlobals, bool forTest)
    : TThread(ThreadProc, reinterpret_cast<void*>(this))
    , TimeToDie(false)
    , Delay(3) /* 3 - это интервал обновления статистики */
    , logStat(nullptr)
{
    if (workerGlobals->ResStatFile.empty()) {
        logStat = fopen("/dev/null", "w");
    }
    else {
        logStat = fopen(workerGlobals->ResStatFile.data(), "a");
        if (logStat) {
            setvbuf(logStat, (char *) nullptr, _IOLBF, 0);
        } else {
            logStat = stderr;
        }
        fprintf(logStat, "#\ttime\tcpu\tcpuavg\tmem\tio\tio_mbps\n");
    }
    monitoredGroups.Reset(new TMap<pid_t, struct resUsageStat_st*>);

    if (!forTest) { // Got segmentation faults and strange aborts when TResourceMonitor was started in unit tests
        Start();
        Detach();
    }
}

TResourceMonitor::~TResourceMonitor()
{
    TimeToDie = true;

    if (logStat && logStat != stderr)
        fclose(logStat);
}

