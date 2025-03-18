#pragma once

#include <util/generic/ptr.h>
#include <util/system/mutex.h>
#include <util/system/mem_info.h>

namespace NStat {
    class TMemCounter {
    public:
        TMemCounter(size_t& trg, TMutex* mutex = nullptr)
            : Target(trg)
            , Lock(mutex ? new TGuard<TMutex>(mutex) : nullptr) // lock if @mutex is supplied
            , StartMem(NMemInfo::GetMemInfo())
        {
        }

        ~TMemCounter() {
            Target = MemoryUsed();
        }

        // Memory usage since creating
        size_t MemoryUsed() const {
            NMemInfo::TMemInfo curMem = NMemInfo::GetMemInfo();
            i64 rss = Max<i64>(curMem.RSS - StartMem.RSS, 0);
            i64 vms = Max<i64>(curMem.VMS - StartMem.VMS, 0);
            return Max(rss, vms);
        }

    private:
        size_t& Target;
        THolder<TGuard<TMutex>> Lock;
        NMemInfo::TMemInfo StartMem;
    };

}
