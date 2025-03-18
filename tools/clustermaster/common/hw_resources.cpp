#include "hw_resources.h"

#include "cgroups.h"

#include <tools/clustermaster/common/log.h>

#include <util/datetime/base.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

#if defined (__FreeBSD__)

#include <stdlib.h>

#endif

#if defined (__FreeBSD__) || defined (__linux__)
double getIdlePercentage(int cnt, long *newStat, long *oldStat)
{
    long totalChange = 0;
    for (int i = 0; i < cnt; i++) {
        totalChange += (newStat[i] - oldStat[i]);
    }

    if (totalChange == 0) {
        totalChange = 1;
    }

    /* Idle is the last member */
    return double(newStat[CPUSTATES - 1] - oldStat[CPUSTATES - 1]) / totalChange;
}
#endif

#if defined (__FreeBSD__)
static double pctdouble (ui32 cpu) {
    return (double)(cpu) / FSCALE;
}
#endif

void THWResources::determineCPUNumber()
{
#if defined (__FreeBSD__) || defined (__linux__)
    hwInfo.nCPU = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );

    hwInfo.nCPU = sysinfo.dwNumberOfProcessors;
#endif
}

int THWResources::determinePageSize()
{
#if defined (__FreeBSD__) || defined (__linux__)
    return sysconf(_SC_PAGESIZE);
#else /* _win_ */
//#error Not implemented yet
    return 0;
#endif
}

void THWResources::determineMemTotal()
{
    hwInfo.pageSize = determinePageSize();

#if defined __FreeBSD__
    int mib[2] = {CTL_HW, HW_PHYSMEM};
    size_t len = sizeof(ui64);
    ui64 memTotal;

    if (sysctl(mib, 2, &memTotal, &len, NULL, 0) == -1) {
        ythrow yexception() << __FUNCTION__ << " : " << __LINE__;
    }

    hwInfo.maxMemPages = memTotal / hwInfo.pageSize * AVAILABLE_MEM;
#elif defined __linux__
    struct sysinfo sysInfo;
    sysinfo(&sysInfo);

    try {
        // limit_in_bytes can be enormous and has nothing with real memory size
        size_t maxMemBytes = GetCgroupVariable("memory", "memory.limit_in_bytes");
        DEBUGLOG("cgroups memory.limit_in_bytes=" << maxMemBytes);
        if (maxMemBytes < sysInfo.totalram) {
            sysInfo.totalram = maxMemBytes;
        }
    }
    catch (const yexception& e) {
        DEBUGLOG("determineMemTotal(): " << e.what());
    }

    hwInfo.maxMemPages = sysInfo.totalram / hwInfo.pageSize * AVAILABLE_MEM;
#endif
}

#ifdef __FreeBSD__
void THWResources::determineBSDVer()
{
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0};
    size_t len = 0;

    mib[3] = getpid();

    if (sysctl(mib, 4, NULL, &len, NULL, 0) == -1) {
        ythrow yexception() << __FUNCTION__ << " : " << __LINE__;
    }

    if (len == sizeof (kinfo_proc_bsd7)) {
        _bsd_ver = 7;
    } else if (len == sizeof (kinfo_proc_bsd8)) {
        _bsd_ver = 8;
    } else {
        _bsd_ver = 0;
        ythrow yexception() << "Unknown FreeBSD version!";
    }
}
#endif

int THWResources::getCPULoad(double* cpuAvg)
{
#if defined (__FreeBSD__) || defined (__linux__)
    int ret;

    ret = getloadavg(cpuAvg, 1);

    *cpuAvg /= hwInfo.nCPU;

    return ret;
#else
//#error Not implemented yet
    return 0;
#endif
}

int THWResources::getCPUUse(double* cpuUse)
{
#if defined __FreeBSD__ || defined __linux__
    static long cp_time[CPUSTATES];
    static long cp_old[CPUSTATES];

    size_t len = sizeof(long)* CPUSTATES;

#if defined __FreeBSD__
    if (sysctlbyname("kern.cp_time", cp_time, &len, NULL, 0) == -1) {
        *cpuUse = 0;
        return 1;
    }
#elif defined __linux__
    char CommandLinePath[] = "/proc/stat";
    FILE* fd_CmdLineFile = fopen (CommandLinePath, "rt");  // open the file for reading text
    if (fd_CmdLineFile) {
        char cpuName[5];
        int read = fscanf(fd_CmdLineFile, "%s  %ld %ld %ld %ld",
            cpuName, &cp_time[0], &cp_time[1], &cp_time[2], &cp_time[3]);
        if (read != 5) {
            Cout << strerror(errno);
            *cpuUse = 0;
            return 1;
        }
        fclose(fd_CmdLineFile);
    } else {
        Cout << strerror(errno);
        *cpuUse = 0;
        return 1;
    }
#endif /* __linux__ */
    *cpuUse = 1.00 - getIdlePercentage(CPUSTATES, cp_time, cp_old);

    /* Move new statistics to old data */
    memcpy(cp_old, cp_time, len);

#elif defined _win_ || defined _darwin_
    *cpuUse = (double) 1/hwInfo.nCPU; /* Use 1 core */
#else
#error
#endif
    return 0;
}

void THWResources::getHWInfo(struct machineInfo* machInfo)
{
    *machInfo = hwInfo;
}

/*
 * returns num of free PAGES, need to rename
 */
int THWResources::getFreeMem()
{
#if defined __FreeBSD__
    /*
     * in FreeBSD free memory calculates as sum of:
     */
    size_t freePagesCount, inactivePagesCount, cachePagesCount;
    size_t len = sizeof(size_t);

    /*
     * FixMe : use mib instead of name
     */
    if (sysctlbyname("vm.stats.vm.v_free_count", &freePagesCount, &len, NULL, 0) == -1) {
        return 0;
    }
    if (sysctlbyname("vm.stats.vm.v_inactive_count", &inactivePagesCount, &len, NULL, 0) == -1) {
        return 0;
    }
    if (sysctlbyname("vm.stats.vm.v_cache_count", &cachePagesCount, &len, NULL, 0) == -1) {
        return 0;
    }

    return freePagesCount + inactivePagesCount + cachePagesCount;
#elif defined __linux__
    struct sysinfo sysInfo;
    sysinfo(&sysInfo);

    try {
        // limit_in_bytes can be enormous and has nothing with real memory size
        size_t maxMemBytes = GetCgroupVariable("memory", "memory.limit_in_bytes");
        DEBUGLOG("cgroups memory.limit_in_bytes=" << maxMemBytes);
        if (maxMemBytes > sysInfo.totalram) {
            maxMemBytes = sysInfo.totalram;
        }
        size_t usedMemBytes = GetCgroupVariable("memory", "memory.usage_in_bytes");
        DEBUGLOG("cgroups memory.usage_in_bytes=" << usedMemBytes);
        return (maxMemBytes - usedMemBytes) / hwInfo.pageSize;
    }
    catch (const yexception& e) {
        DEBUGLOG("getFreeMem(): " << e.what());
    }

    /* FixMe - check, may be need to sum somewhat (shared, buffer)??*/
    return ((sysInfo.freeram + sysInfo.bufferram) / hwInfo.pageSize);
#elif defined WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys / hwInfo.pageSize;
#endif
}

int THWResources::updateIOStat(long double* IO_Busy, long double* IO_MBpS)
{
#if defined __FreeBSD__
    /*
     * FixMe: Таким образом не работает, поскольку этот devid и devstat' devid
     * не совпадают. Т.е. нужен другой способ идентификации нужного девайса
     */
    devstat_getdevs(NULL, &currIOStat);

    long double etime = currIOStat.snap_time - prevIOStat.snap_time;
    if (etime == 0.0)
        etime = 1.0;

    if (devstat_compute_statistics(&currIOStat.dinfo->devices[0],
                                   &prevIOStat.dinfo->devices[0],
                                   etime,
                                   DSM_MB_PER_SECOND, IO_MBpS,
                                   DSM_BUSY_PCT, IO_Busy,
                                   DSM_NONE) != 0) {
    }

    /* обновим старые данные */
    struct devinfo *tmp_devstat = prevIOStat.dinfo;
    prevIOStat.dinfo = currIOStat.dinfo;
    currIOStat.dinfo = tmp_devstat;
    prevIOStat.snap_time = currIOStat.snap_time;

    return 0;
#else
//#warning Need to implement IO statistics
    /*
     * FixMe!
     */
    *IO_Busy = 0;
    *IO_MBpS = 0;
    return 0;
#endif
}

#if defined __FreeBSD__
template <typename T>
int THWResources::_getProcessGroupInfo(pid_t pgid, TVector<KInfoCommonPtr>* procGroupInfo)
{
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PGRP, pgid };
    size_t len = 0;

    if (sysctl(mib, 4, NULL, &len, NULL, 0) == -1) {
        return 0;
    }

    char* buf = new char[len];

    if ((sysctl(mib, 4, (void *)buf, &len, NULL, 0) == 0) && (len != 0)) {
        T* pp = reinterpret_cast<T *>(buf);
        size_t kinfoStructRealSize = pp->ki_structsize;
        size_t nproc = len / kinfoStructRealSize;
        for (size_t i = 0; i < nproc; ++i, ++pp) {
            KInfoCommonPtr procInfo = new TInfoCommon;

            procInfo->ki_pid = pp->ki_pid;
            procInfo->ki_pgid = pp->ki_pgid;
            procInfo->ki_rssize = 0;
            /* Take formula from top.c;
             * Get load per core, so need to add factor 1/number_of_cores
             */
            procInfo->ki_cpuavg = (pp->ki_swtime == 0 ? 0 :
                    (pctdouble(pp->ki_pctcpu) / hwInfo.nCPU ));

            if (pp->ki_rssize > MIN_RSS_PAGE_COUNT) {
                GetDetailedMemInfo(pp->ki_pid, procInfo->SharedMemAreas, procInfo->ki_rssize);
            } else {
                procInfo->ki_rssize = pp->ki_rssize;
            }

            procGroupInfo->push_back(procInfo);
        }
        delete[] buf;
        return nproc;
    } else {
        delete[] buf;
        return 0;
    }
}
#elif defined __linux__
int THWResources::calcProcessCpuAvg(kinfo_proc_linux* procInfo)
{
    ui64 used_ticks, avail_ticks, ticks_after_boot;

    TString uptimeStr;
    TUnbufferedFileInput in("/proc/uptime");
    uptimeStr = in.ReadLine();
    double up, idle;
    if (sscanf(uptimeStr.data(), "%lf %lf", &up, &idle) < 2) {
        ythrow yexception() << "cannot read values from /proc/uptime";
    }

    ticks_after_boot = (ui64)(up * HZ);

    avail_ticks = (ticks_after_boot - procInfo->start_time) * hwInfo.nCPU;
    used_ticks = procInfo->utime + procInfo->stime;
    if (avail_ticks)
        procInfo->pcpu = (double)(used_ticks)/avail_ticks;
    else
        procInfo->pcpu = 0.00;

    return 0;
}

void THWResources::fullProcStatParse(const char* S, kinfo_proc_linux * P) {
    ui32 num;

    num = sscanf(S, "%d ", &P->ppid);

/* WARNING: Don't copy cmd for time economy */
#if 0
#define unlikely(x)     __builtin_expect(!!(x),0)

    const char* tmp;
    S = strchr(S, '(') + 1;
    tmp = strrchr(S, ')');
    num = tmp - S;
    if (unlikely(num >= sizeof P->cmd))
        num = sizeof P->cmd - 1;
    memcpy(P->cmd, S, num);
    P->cmd[num] = '\0';
    S = tmp + 2;                            /* skip ") "  */
#else
    S = strrchr(S, ')') + 2;                /* skip ") "  */
#endif

    num = sscanf(S,
        "%c "
        "%d %d %d %d %d "
        "%lu %lu %lu %lu %lu "
        "%lu %lu %ld %ld "  /* utime stime cutime cstime */
        "%ld %ld "
        "%d "
        "%ld "
        "%Lu "  /* start_time */
        "%lu "
        "%ld "
        "%lu %lu %lu %lu %lu %lu "
        "%*s %*s %*s %*s " /* discard, no RT signals & Linux 2.1 used hex */
        "%lu %*u %*u "
        "%d %d "
        "%lu %lu",
        &P->state,
        &P->ppid, &P->pgrp, &P->session, &P->tty, &P->tpgid,
        &P->flags, &P->min_flt, &P->cmin_flt, &P->maj_flt, &P->cmaj_flt,
        &P->utime, &P->stime, &P->cutime, &P->cstime,
        &P->priority, &P->nice,
        &P->nlwp,
        &P->alarm,
        &P->start_time,
        &P->vsize,
        &P->rss,
        &P->rss_rlim, &P->start_code, &P->end_code, &P->start_stack, &P->kstk_esp, &P->kstk_eip,
    /*     P->signal, P->blocked, P->sigignore, P->sigcatch,   */ /* can't use */
        &P->wchan, /* &P->nswap, &P->cnswap, */  /* nswap and cnswap dead for 2.4.xx and up */
    /* -- Linux 2.0.35 ends here -- */
        &P->exit_signal, &P->processor,  /* 2.2.1 ends with "exit_signal" */
    /* -- Linux 2.2.8 to 2.5.17 end here -- */
        &P->rtprio, &P->sched  /* both added to 2.5.18 */
    );
    Y_UNUSED(num);

    if(!P->nlwp){
        P->nlwp = 1;
    }
    calcProcessCpuAvg(P);
}

static pid_t getPGRP(const char* S)
{
    const char* tmp;

    tmp = strrchr(S, ')');
    S = tmp + 2;                 // skip ") "

    char state;
    int ppid, pgrp;
    sscanf(S,
            "%c "
            "%d %d",
            &state,
            &ppid, &pgrp);

    return pgrp;
}

static int IsNumeric(const char* ccharptr_CharacterList)
{
    for ( ; *ccharptr_CharacterList; ccharptr_CharacterList++)
        if (*ccharptr_CharacterList < '0' || *ccharptr_CharacterList > '9')
            return false;

    return true;
}

#define PROC_DIRECTORY "/proc/"
int THWResources::_getProcessGroupInfo(TMap<pid_t, TVector<KInfoCommonPtr> >* procGroupInfo)
{
    char CommandLinePath[100];
    struct dirent* de_DirEntity = nullptr;
    DIR* dir_proc = nullptr;

    size_t n = 4096;
    char *buf = (char*)malloc(n);

    dir_proc = opendir(PROC_DIRECTORY);
    if (dir_proc == nullptr) {
        perror("Couldn't open the " PROC_DIRECTORY " directory");
        return -1;
    }

    // Loop while not NULL
    while ( (de_DirEntity = readdir(dir_proc)) )
        if ((de_DirEntity->d_type == DT_DIR) && IsNumeric(de_DirEntity->d_name)) {
            strcpy(CommandLinePath, PROC_DIRECTORY);
            strcat(CommandLinePath, de_DirEntity->d_name);
            strcat(CommandLinePath, "/stat");
            FILE* fd_CmdLineFile = fopen (CommandLinePath, "rt");  // open the file for reading text
            if (fd_CmdLineFile) {
                int read = getline(&buf, &n, fd_CmdLineFile);
                if (read != -1) {
                    pid_t curPGRP = getPGRP(buf);
                    if (procGroupInfo->find(curPGRP) != procGroupInfo->end()) {
                        kinfo_proc_linux P;
                        fullProcStatParse(buf, &P);
                        KInfoCommonPtr res = new TInfoCommon;
                        res->ki_pid    = P.ppid;
                        res->ki_pgid   = P.pgrp;
                        res->ki_cpuavg = P.pcpu;
                        res->ki_rssize = P.rss;

                        (*procGroupInfo)[curPGRP].push_back(res);
                    }
                } else {
                    Cout << strerror(errno);
                }
            fclose(fd_CmdLineFile);
            } else {
                /* Do nothing, probably process died */
            }
        }

    closedir(dir_proc);

    free(buf);

    return 0;
}

#endif
int THWResources::getProcessGroupInfo(pid_t pgid, TVector<KInfoCommonPtr>* procGroupInfo)
{
#if defined __FreeBSD__
    if (_bsd_ver == 7) {
        return _getProcessGroupInfo<struct kinfo_proc_bsd7>(pgid, procGroupInfo);
    } else if (_bsd_ver == 8) {
        return _getProcessGroupInfo<struct kinfo_proc_bsd8>(pgid, procGroupInfo);
    }

    return 0;
#elif defined __linux__
    TMap<pid_t, TVector<KInfoCommonPtr> > tmp;
    TVector<KInfoCommonPtr> emptyStat;
    tmp[pgid] = emptyStat;
    getProcessGroupInfo(&tmp);
    *procGroupInfo = emptyStat;
    return 0;
#elif defined _win_
    return 0;
#endif
}

int THWResources::getProcessGroupInfo(TMap<pid_t, TVector<KInfoCommonPtr> >* procGroupInfos)
{
#if defined __FreeBSD__
    for (TMap<pid_t, TVector<KInfoCommonPtr> >::iterator it = procGroupInfos->begin();
            it != procGroupInfos->end(); ++it) {
        getProcessGroupInfo(it->first, &it->second);
    }

    return 0;
#elif defined __linux__
    return _getProcessGroupInfo(procGroupInfos);
#elif defined _win_
    return 0;
#endif
}

int THWResources::GetDetailedMemInfo (pid_t pid, TSimpleSharedPtr<TMap<TString, ui32> >& sharedMemAreas, ui64& rssPages)
{
#if defined __FreeBSD__
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_VMMAP, pid};
    size_t len = 0;
    char* buf;

    if (sysctl(mib, 4, NULL, &len, NULL, 0) == -1) {
        return 0;
    }

    buf = new char[len];
    if (sysctl(mib, 4, (void *)buf, &len, NULL, 0) == -1) {
        delete[] buf;
        return 0;
    }

    size_t shift = 0;
    while (shift < len) {
        struct kinfo_vmentry* pp = reinterpret_cast<struct kinfo_vmentry *>(buf + shift);
        size_t kinfoEntryRealSize = pp->kve_structsize;

        if (pp->kve_type == KVME_TYPE_DEFAULT) {
            rssPages += pp->kve_resident;
        } else if (pp->kve_type == KVME_TYPE_SWAP) {
            rssPages += (pp->kve_end - pp->kve_start)/hwInfo.pageSize;
        } else if ((pp->kve_type == KVME_TYPE_VNODE) && (pp->kve_resident > MIN_SHARED_BLOCK_PAGE_COUNT) &&
                (pp->kve_path[0] != '\0') && (pp->kve_path[0] != '-')) {
            (*sharedMemAreas)[TString(pp->kve_path)] = pp->kve_resident;
        }

        shift += kinfoEntryRealSize;
    }

    delete[] buf;
    return sharedMemAreas->size();
#elif defined __linux__
    Y_UNUSED(rssPages);
    TString procMapFile = "/proc/" + ToString<pid_t>(pid) + "/maps";

    try {
        TFileInput procMapFd(procMapFile);
        char sharedAreaName[PATH_MAX];
        TString line;
        while (procMapFd.ReadLine(line)) {
            ui64 tmp, startAddr, endAddr;
            char useless[10];

            if ((sscanf(line.data(), "%lx-%lx %s %ld %s %ld %s\n",
                            &startAddr, &endAddr, useless,
                            &tmp, useless, &tmp, sharedAreaName)) == 7) {
                ui32 pageCount = (endAddr - startAddr) / hwInfo.pageSize;
                (*sharedMemAreas)[TString(sharedAreaName)] += pageCount;
            }
        }
    } catch (const TFileError& e) {;}

    return sharedMemAreas->size();
#elif defined _win_
    Y_UNUSED(pid);
    Y_UNUSED(sharedMemAreas);
    Y_UNUSED(rssPages);
    return 0;
#endif
}

THWResources::THWResources()
{
    try {
        determineCPUNumber();
        determineMemTotal();
#ifdef __FreeBSD__
        determineBSDVer();
        /* For I/O statistics */
        currIOStat.dinfo = (struct devinfo *) calloc(1, sizeof(struct devinfo));
        prevIOStat.dinfo = (struct devinfo *) calloc(1, sizeof(struct devinfo));
#endif
    } catch(...) {
        throw;
    }
}

THWResources::~THWResources() {
#ifdef __FreeBSD__
    free(prevIOStat.dinfo);
    free(currIOStat.dinfo);
#endif
}
