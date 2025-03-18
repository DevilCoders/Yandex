#pragma once

/*
 * This module implements collecting of system information
 * and global resource usage statistics.
 *
 * Currently FreeBSD 7.x, 8.x are the only supported systems.
 */

#include <sys/types.h>

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <errno.h>
#include <math.h>
#include <stdio.h>

#if defined __linux__

#include <sys/resource.h>
#include <sys/sysinfo.h>

#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#elif defined WIN32

#include <util/system/winint.h>

#elif defined __FreeBSD__

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <dlfcn.h>
#include <paths.h>
#include <unistd.h>

/* IO stat */

#include <vm/vm_param.h>

#include <devstat.h>

#endif

struct machineInfo
{
    ui32    nCPU;
    ui32    maxMemPages;    /* Total system memory in pages */
    ui32    pageSize;       /* hw page size, 4k usually */
};

class TRUsage: public rusage {
public:
    inline size_t getIOBlocksCount() const {
        return ru_inblock + ru_oublock + ru_majflt;
    };
};

#define AVAILABLE_MEM       0.90
typedef TSimpleSharedPtr<TMap<TString, ui32> > TMemSharedAreasPtr;

struct TInfoCommon
{
    pid_t   ki_pid;
    pid_t   ki_pgid;
    double  ki_cpuavg;
    ui64    ki_rssize;      /* current resident set size in pages */
    TMemSharedAreasPtr   SharedMemAreas;

    TInfoCommon() :
        ki_pid(0)
        , ki_pgid(0)
        , ki_cpuavg(0.00)
        , ki_rssize(0)
        , SharedMemAreas(new TMap<TString, ui32>) {};
};

typedef TSimpleSharedPtr<TInfoCommon> KInfoCommonPtr;

#ifdef __linux__
#define CPUSTATES   4
#endif /* __linux__*/

class THWResources {
public:
    void getHWInfo(struct machineInfo* machInfo);
    /* Average system load for 1-minute period */
    int getCPULoad(double* cpuAvg);
    /* Average system load for period from last call*/
    int getCPUUse(double* cpuUse);
    int getFreeMem();
    int updateIOStat(long double* IO_Busy, long double* IO_MBpS);

    int getProcessGroupInfo(pid_t pgid, TVector<KInfoCommonPtr>* procGroupInfo);
    int getProcessGroupInfo(TMap<pid_t, TVector<KInfoCommonPtr> >* procGroupInfos);
    int GetDetailedMemInfo (pid_t pid, TSimpleSharedPtr<TMap<TString, ui32> >& sharedMemAreas, ui64& rssPages);
    THWResources();
    ~THWResources();

protected:
    struct machineInfo hwInfo;
private:

    static const i32 MIN_RSS_PAGE_COUNT = 102400;
    static const i32 MIN_SHARED_BLOCK_PAGE_COUNT = 51200;

#ifdef __FreeBSD__
    /*
     * Проблема заключается в отсутствии кросс-компиляции при сборке Аркадии
     * В данное время система компилится на системах FreeBSD 7.2, а запускается
     * в последствии на системах FreeBSD 8.2, и, возможно, FreeBSD 7.2
     * использовать, к примеру, kvmlib нельзя - она неправильно бэкпортирована.
     * Поэтому, в рантайме необходимо определить версию системы - от этого зависит,
     * какая структура kinfo_proc будет возвращаться системой. Собственно, по размеру
     * возвращаемой структуры мы и будет это определять.
     *
     * Видимо при полном переезде на 8.2 данная проблема отпадет, но может появиться
     * с появлением FreeBSD 9.x
     *
     * обратить внимание на
     *  struct kinfo_proc
     *  struct kinfo_vmentry
     */
    int _bsd_ver;

    #define KI_NGROUPS_BSD8 16
    #define KI_NGROUPS_BSD7 64


    /*
     * Данная структура объявлена правильно до поля ki_swtime,
     * что дает доступ к необходимым нам структурам для подсчета статистики
     * Поскольку далее используется энное кол-во системо-специфичных объявлений
     * которые переобъявлеть лень, добиваем размер паддингом.
     */
    struct kinfo_proc_bsd8 {
        int ki_structsize;      /* size of this structure */
        int ki_layout;      /* reserved: layout identifier */
        struct  pargs *ki_args;     /* address of command arguments */
        struct  proc *ki_paddr;     /* address of proc */
        struct  user *ki_addr;      /* kernel virtual addr of u-area */
        struct  vnode *ki_tracep;   /* pointer to trace file */
        struct  vnode *ki_textvp;   /* pointer to executable file */
        struct  filedesc *ki_fd;    /* pointer to open file info */
        struct  vmspace *ki_vmspace;    /* pointer to kernel vmspace struct */
        void    *ki_wchan;      /* sleep address */
        pid_t   ki_pid;         /* Process identifier */
        pid_t   ki_ppid;        /* parent process id */
        pid_t   ki_pgid;        /* process group id */
        pid_t   ki_tpgid;       /* tty process group id */
        pid_t   ki_sid;         /* Process session ID */
        pid_t   ki_tsid;        /* Terminal session ID */
        short   ki_jobc;        /* job control counter */
        short   ki_spare_short1;    /* unused (just here for alignment) */
        dev_t   ki_tdev;        /* controlling tty dev */
        sigset_t ki_siglist;        /* Signals arrived but not delivered */
        sigset_t ki_sigmask;        /* Current signal mask */
        sigset_t ki_sigignore;      /* Signals being ignored */
        sigset_t ki_sigcatch;       /* Signals being caught by user */
        uid_t   ki_uid;         /* effective user id */
        uid_t   ki_ruid;        /* Real user id */
        uid_t   ki_svuid;       /* Saved effective user id */
        gid_t   ki_rgid;        /* Real group id */
        gid_t   ki_svgid;       /* Saved effective group id */
        short   ki_ngroups;     /* number of groups */
        short   ki_spare_short2;    /* unused (just here for alignment) */

        gid_t   ki_groups[KI_NGROUPS_BSD8];  /* groups */
        vm_size_t ki_size;      /* virtual size */
        segsz_t ki_rssize;      /* current resident set size in pages */
        segsz_t ki_swrss;       /* resident set size before last swap */
        segsz_t ki_tsize;       /* text size (pages) XXX */
        segsz_t ki_dsize;       /* data size (pages) XXX */
        segsz_t ki_ssize;       /* stack size (pages) */
        u_short ki_xstat;       /* Exit status for wait & stop signal */
        u_short ki_acflag;      /* Accounting flags */
        fixpt_t ki_pctcpu;      /* %cpu for process during ki_swtime */
        u_int   ki_estcpu;      /* Time averaged value of ki_cpticks */
        u_int   ki_slptime;     /* Time since last blocked */
        u_int   ki_swtime;      /* Time swapped in or out */

        char    padding[760];
    };

    /*
     * Данная структура объявлена полностью честно, потому что мы и собираем все
     * на данной версии.
     */
    struct kinfo_proc_bsd7 {
        int ki_structsize;      /* size of this structure */
        int ki_layout;      /* reserved: layout identifier */
        struct  pargs *ki_args;     /* address of command arguments */
        struct  proc *ki_paddr;     /* address of proc */
        struct  user *ki_addr;      /* kernel virtual addr of u-area */
        struct  vnode *ki_tracep;   /* pointer to trace file */
        struct  vnode *ki_textvp;   /* pointer to executable file */
        struct  filedesc *ki_fd;    /* pointer to open file info */
        struct  vmspace *ki_vmspace;    /* pointer to kernel vmspace struct */
        void    *ki_wchan;      /* sleep address */
        pid_t   ki_pid;         /* Process identifier */
        pid_t   ki_ppid;        /* parent process id */
        pid_t   ki_pgid;        /* process group id */
        pid_t   ki_tpgid;       /* tty process group id */
        pid_t   ki_sid;         /* Process session ID */
        pid_t   ki_tsid;        /* Terminal session ID */
        short   ki_jobc;        /* job control counter */
        short   ki_spare_short1;    /* unused (just here for alignment) */
        dev_t   ki_tdev;        /* controlling tty dev */
        sigset_t ki_siglist;        /* Signals arrived but not delivered */
        sigset_t ki_sigmask;        /* Current signal mask */
        sigset_t ki_sigignore;      /* Signals being ignored */
        sigset_t ki_sigcatch;       /* Signals being caught by user */
        uid_t   ki_uid;         /* effective user id */
        uid_t   ki_ruid;        /* Real user id */
        uid_t   ki_svuid;       /* Saved effective user id */
        gid_t   ki_rgid;        /* Real group id */
        gid_t   ki_svgid;       /* Saved effective group id */
        short   ki_ngroups;     /* number of groups */
        short   ki_spare_short2;    /* unused (just here for alignment) */
        gid_t   ki_groups[KI_NGROUPS_BSD7];  /* groups */
        vm_size_t ki_size;      /* virtual size */
        segsz_t ki_rssize;      /* current resident set size in pages */
        segsz_t ki_swrss;       /* resident set size before last swap */
        segsz_t ki_tsize;       /* text size (pages) XXX */
        segsz_t ki_dsize;       /* data size (pages) XXX */
        segsz_t ki_ssize;       /* stack size (pages) */
        u_short ki_xstat;       /* Exit status for wait & stop signal */
        u_short ki_acflag;      /* Accounting flags */
        fixpt_t ki_pctcpu;      /* %cpu for process during ki_swtime */
        u_int   ki_estcpu;      /* Time averaged value of ki_cpticks */
        u_int   ki_slptime;     /* Time since last blocked */
        u_int   ki_swtime;      /* Time swapped in or out */
        int ki_spareint1;       /* unused (just here for alignment) */
        u_int64_t ki_runtime;       /* Real time in microsec */
        struct  timeval ki_start;   /* starting time */
        struct  timeval ki_childtime;   /* time used by process children */
        long    ki_flag;        /* P_* flags */
        long    ki_kiflag;      /* KI_* flags (below) */
        int ki_traceflag;       /* Kernel trace points */
        char    ki_stat;        /* S* process status */
        signed char ki_nice;        /* Process "nice" value */
        char    ki_lock;        /* Process lock (prevent swap) count */
        char    ki_rqindex;     /* Run queue index */
        u_char  ki_oncpu;       /* Which cpu we are on */
        u_char  ki_lastcpu;     /* Last cpu we were on */
        char    ki_ocomm[OCOMMLEN+1];   /* thread name */
        char    ki_wmesg[WMESGLEN+1];   /* wchan message */
        char    ki_login[LOGNAMELEN+1]; /* setlogin name */
        char    ki_lockname[LOCKNAMELEN+1]; /* lock name */
        char    ki_comm[COMMLEN+1]; /* command name */
        char    ki_emul[KI_EMULNAMELEN+1];  /* emulation name */
        /*
         * When adding new variables, take space for char-strings from the
         * front of ki_sparestrings, and ints from the end of ki_spareints.
         * That way the spare room from both arrays will remain contiguous.
         */
        char    ki_sparestrings[68];    /* spare string space */
        int ki_spareints[KI_NSPARE_INT];    /* spare room for growth */
        int ki_jid;         /* Process jail ID */
        int ki_numthreads;      /* XXXKSE number of threads in total */
        lwpid_t ki_tid;         /* XXXKSE thread id */
        struct  priority ki_pri;    /* process priority */
        struct  rusage ki_rusage;   /* process rusage statistics */
        /* XXX - most fields in ki_rusage_ch are not (yet) filled in */
        struct  rusage ki_rusage_ch;    /* rusage of children processes */
        struct  pcb *ki_pcb;        /* kernel virtual addr of pcb */
        void    *ki_kstack;     /* kernel virtual addr of stack */
        void    *ki_udata;      /* User convenience pointer */
        /*
         * When adding new variables, take space for pointers from the
         * front of ki_spareptrs, and longs from the end of ki_sparelongs.
         * That way the spare room from both arrays will remain contiguous.
         */
        void    *ki_spareptrs[KI_NSPARE_PTR];   /* spare room for growth */
        long    ki_sparelongs[KI_NSPARE_LONG];  /* spare room for growth */
        long    ki_sflag;       /* PS_* flags */
        long    ki_tdflags;     /* XXXKSE kthread flag */
    };

    /* For IO statistics */
    struct statinfo currIOStat, prevIOStat;

#elif defined __linux__
    // Basic data structure which holds all information we can get about a process.
// (unless otherwise specified, fields are read from /proc/#/stat)
//
// Most of it comes from task_struct in linux/sched.h
//
#define P_G_SZ 20
    typedef struct kinfo_proc_linux {
// 1st 16 bytes
    int
    tid,        // (special)       task id, the POSIX thread ID (see also: tgid)
    ppid;       // stat,status     pid of parent process
    double
        pcpu;   // stat (special)  %CPU usage
    char
    state,      // stat,status     single-char code for process state (S=sleeping)
    pad_1,      // n/a             padding
    pad_2,      // n/a             padding
    pad_3;      // n/a             padding
// 2nd 16 bytes
    unsigned long
    utime,      // stat            user-mode CPU time accumulated by process
    stime;      // stat            kernel-mode CPU time accumulated by process
    long
// and so on...
    cutime,     // stat            cumulative utime of process and reaped children
    cstime;     // stat            cumulative stime of process and reaped children
    unsigned long long
    start_time; // stat            the time the process started after system boot
    unsigned long
    start_code, // stat            address of beginning of code segment
    end_code,   // stat            address of end of code segment
    start_stack,    // stat            address of the bottom of stack for the process
    kstk_esp,   // stat            kernel stack pointer
    kstk_eip,   // stat            kernel instruction pointer
    wchan;      // stat (special)  address of kernel wait channel proc is sleeping in
    long
    priority,   // stat            kernel scheduling priority
    nice,       // stat            standard unix nice level of process
    rss,        // stat            resident set size from /proc/#/stat (pages)
    alarm,      // stat            ?
    // the next 7 members come from /proc/#/statm
    size,       // statm           total # of pages of memory
    resident,   // statm           number of resident set (non-swapped) pages (4k)
    share,      // statm           number of pages of shared (mmap'd) memory
    trs,        // statm           text resident set size
    lrs,        // statm           shared-lib resident set size
    drs,        // statm           data resident set size
    dt;     // statm           dirty pages
    unsigned long
    vm_size,        // status          same as vsize in kb
    vm_lock,        // status          locked pages in kb
    vm_rss,         // status          same as rss in kb
    vm_data,        // status          data size
    vm_stack,       // status          stack size
    vm_swap,        // status          based on "swap ents", Linux 2.6.34
    vm_exe,         // status          executable size
    vm_lib,         // status          library size (all pages, not just used ones)
    rtprio,     // stat            real-time priority
    sched,      // stat            scheduling class
    vsize,      // stat            number of pages of virtual memory ...
    rss_rlim,   // stat            resident set size limit?
    flags,      // stat            kernel flags for the process
    min_flt,    // stat            number of minor page faults since process start
    maj_flt,    // stat            number of major page faults since process start
    cmin_flt,   // stat            cumulative min_flt of process and child processes
    cmaj_flt;   // stat            cumulative maj_flt of process and child processes
    char
    **environ,  // (special)       environment string vector (/proc/#/environ)
    **cmdline;  // (special)       command line string vector (/proc/#/cmdline)
    char
    // Be compatible: Digital allows 16 and NT allows 14 ???
    euser[P_G_SZ],  // stat(),status   effective user name
    ruser[P_G_SZ],  // status          real user name
    suser[P_G_SZ],  // status          saved user name
    fuser[P_G_SZ],  // status          filesystem user name
    rgroup[P_G_SZ], // status          real group name
    egroup[P_G_SZ], // status          effective group name
    sgroup[P_G_SZ], // status          saved group name
    fgroup[P_G_SZ], // status          filesystem group name
    cmd[16];    // stat,status     basename of executable file in call to exec(2)
    int
    pgrp,       // stat            process group id
    session,    // stat            session id
    nlwp,       // stat,status     number of threads, or 0 if no clue
    tgid,       // (special)       task group ID, the POSIX PID (see also: tid)
    tty,        // stat            full device number of controlling terminal
    euid, egid,     // stat(),status   effective
    ruid, rgid,     // status          real
    suid, sgid,     // status          saved
    fuid, fgid,     // status          fs (used for file access only)
    tpgid,      // stat            terminal process group id
    exit_signal,    // stat            might not be SIGCHLD
    processor;      // stat            current (or most recent?) CPU
} kinfo_proc_linux;
#endif /* __linux__ */

    int  determinePageSize();
    void determineCPUNumber();
    void determineMemTotal();

#if defined __FreeBSD__
    void determineBSDVer();
    template<typename T>
    int _getProcessGroupInfo(pid_t pgid, TVector<KInfoCommonPtr>* procGroupInfo);
    int _getProcessGroupInfo(TMap<pid_t, TVector<KInfoCommonPtr> >* procGroupInfos);
#elif defined __linux__
    int calcProcessCpuAvg(kinfo_proc_linux* procInfo);
    void fullProcStatParse(const char* S, kinfo_proc_linux * P);
    int _getProcessGroupInfo(TMap<pid_t, TVector<KInfoCommonPtr> >* procGroupInfo);
#endif
};
