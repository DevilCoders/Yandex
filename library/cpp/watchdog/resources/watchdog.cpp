#include "watchdog.h"

#include <library/cpp/watchdog/lib/factory.h>

#include <util/generic/singleton.h>

#if defined(_freebsd_)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#elif defined(_linux_)
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/syscall.h>
struct linux_dirent {
    long d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[];
};
#endif

#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/stream/file.h>

#include <stdlib.h>

TResourcesWatchDogHandle::TResourcesWatchDogHandle(size_t maxVirtualMemorySize, int maxFileDescriptors, TDuration delay)
    : MaxVirtualMemorySize(maxVirtualMemorySize)
    , MaxFileDescriptors(maxFileDescriptors)
    , Start(delay.ToDeadLine())
{
#if defined(_linux_)
    Pid = getpid();
    if (maxVirtualMemorySize > 0) {
        statFile = "/proc/";
        statFile += IntToString<10, int>(Pid);
        statFile += "/stat";
    }

    if (maxFileDescriptors > 0) {
        fdDirectory = "/proc/";
        fdDirectory += IntToString<10, int>(Pid);
        fdDirectory += "/fd";
    }
    Buffer.resize(1024 + PATH_MAX); // Will be enough to read /proc/[pid]/stat till vsize value
#elif defined(_freebsd_)
    Pid = getpid();
    Buffer.resize(1024);
#endif
}

void TResourcesWatchDogHandle::Check(TInstant timeNow) {
    if (timeNow < Start) {
        return;
    }

    if (MaxVirtualMemorySize > 0) {
        size_t vmSize = 0;
#if defined(_freebsd_) && defined(KERN_PROC_PID)
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PID;
        mib[3] = Pid;

        struct kinfo_proc buf;
        size_t len = sizeof(buf);
        if (sysctl(mib, 4, (char*)&buf, &len, NULL, 0) == 0) {
            vmSize = buf.ki_size;
        } else {
            Cerr << "Sysctl(kern.proc.pid) failed: " << strerror(errno) << '\n';
            MaxVirtualMemorySize = 0;
        }

#elif defined(_linux_)
        try {
            // Read 23-th field from /proc/[pid]/stat
            TUnbufferedFileInput statInput(statFile);
            size_t len = statInput.Read(&Buffer[0], Buffer.size());
            TVector<char>::reverse_iterator r = Find(Buffer.rend() - len, Buffer.rend(), ')');
            if (r != Buffer.rend()) {
                TVector<char>::iterator end = Buffer.begin() + len;
                // Skip all the values till vmSize
                int skip = 20;
                TVector<char>::iterator symbol = r.base() + 1;
                for (; symbol < end; ++symbol) {
                    if (*symbol == ' ') {
                        --skip;
                        if (skip == 0) {
                            break;
                        }
                    }
                }
                if (skip == 0) {
                    ++symbol;
                    char* endptr;
                    vmSize = strtoul(&*symbol, &endptr, 10);
                } else {
                    Cerr << "Invalid " << statFile << '\n';
                    MaxVirtualMemorySize = 0;
                }
            }
        } catch (...) {
            Cerr << "Can't read " << statFile << '\n';
            MaxVirtualMemorySize = 0;
        }
#endif
        if (vmSize == 0 && MaxVirtualMemorySize != 0) {
            Cerr << "Unable to detect leaks of virtual memory\n";
            MaxVirtualMemorySize = 0;
        } else {
            Y_VERIFY(vmSize <= MaxVirtualMemorySize, "Process uses too much virtual memory");
        }
    }

    if (MaxFileDescriptors > 0) {
        int nFileDescriptors = 0;
#if defined(_freebsd_) && defined(KERN_PROC_FILEDESC)
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_FILEDESC;
        mib[3] = getpid();

        size_t length;
        if (sysctl(mib, 4, NULL, &length, NULL, 0) == 0) {
            length *= 2; // Just in case if data will be significantly increased
            if (Buffer.size() < length) {
                Buffer.resize(length * 2);
            }
            if (sysctl(mib, 4, &Buffer[0], &length, NULL, 0) == 0) {
                char* p = &Buffer[0];
                char* end = &Buffer[0] + length;
                while (p < end) {
                    p += ((struct kinfo_file*)(uintptr_t)p)->kf_structsize;
                    ++nFileDescriptors;
                }
            } else if (errno != ENOMEM) {
                // ENOMEM can be returned if too many file descriptors have been recently allocated
                Cerr << "Sysctl(kern.proc.filedesc) failed: " << strerror(errno) << '\n';
                MaxFileDescriptors = 0;
            }
        } else {
            Cerr << "Sysctl(kern.proc.filedesc) failed: " << strerror(errno) << '\n';
            MaxFileDescriptors = 0;
        }
#elif defined(_linux_)
        // Cout the number of entries in /proc/[pid]/fd
        DIR* dir = opendir(fdDirectory.c_str());
        if (dir != nullptr) {
#if SYS_getdents
            int nRead;
            nRead = syscall(SYS_getdents, dirfd(dir), &Buffer[0], Buffer.size());
            while (nRead > 0) {
                //Cerr << nRead << ", " << Buffer.size() << '\n';
                if (nFileDescriptors > 0 && nRead > 0) {
                    Buffer.resize(Buffer.size() * 2);
                }

                for (int i = 0; i < nRead;) {
                    struct linux_dirent* d = (struct linux_dirent*)(&Buffer[0] + i);
                    i += d->d_reclen;
                    ++nFileDescriptors;
                }
                nRead = syscall(SYS_getdents, dirfd(dir), &Buffer[0], Buffer.size());
            }
#else
            // Shorter POSIX solution but 4-5 times slower
            struct dirent entry;
            struct dirent* result;
            while (readdir_r(dir, &entry, &result) == 0 && result != NULL) {
                ++nFileDescriptors;
            }
#endif
            closedir(dir);
            nFileDescriptors -= 3; // Don't account ., .. and opened /proc/[pid]/fd
        } else {
            Cerr << "Can't open " << fdDirectory << ": " << strerror(errno) << "\n";
            MaxFileDescriptors = 0;
        }
#endif
        Y_VERIFY(nFileDescriptors <= MaxFileDescriptors, "Too many file descriptors opened");
    }
}

IWatchDog* TResourcesWatchDogHandle::Create(size_t maxVirtualMemorySize, int maxFileDescriptors, TDuration delay) {
    TWatchDogHandlePtr handlePtr(new TResourcesWatchDogHandle(maxVirtualMemorySize, maxFileDescriptors, delay));
    return Singleton<TWatchDogFactory>()->RegisterWatchDogHandle(handlePtr);
}
