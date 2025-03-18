#include "md5_mapping.h"

#include <library/cpp/digest/md5/md5.h>

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>
#include <util/generic/yexception.h>
#include <util/thread/pool.h>

#include <utility>

namespace NFilesMD5Mapping {
    const TMD5Info BUSY = {"BUSY", ""};

    enum ECalcState {
        CS_None,
        CS_Scheduled,
        CS_Ready,
    };

    struct TMD5Holder {
        TMD5Info Info;
        TMutex CalcMutex;

        ECalcState CalcState = CS_None;
        TRWMutex CalcStateMutex;
    };

    typedef TAtomicSharedPtr<TMD5Holder> TSharedMD5Holder;
    typedef TMap<TString, const TSharedMD5Holder> TMappings;
    typedef TMappings::iterator TMappingsIter;

    struct TFilesMD5Map {
        TMappings Mappings;
        TRWMutex Mutex;
    };

    TFilesMD5Map& GetMappedFiles() {
        return *Singleton<TFilesMD5Map>();
    }

    TSharedMD5Holder Find(const TString& filename) {
        TFilesMD5Map& fm = GetMappedFiles();
        {
            TReadGuard fmg(fm.Mutex);
            if (auto mapping = fm.Mappings.FindPtr(filename)) {
                return *mapping;
            }
        }

        TWriteGuard fmg(fm.Mutex);
        if (auto mapping = fm.Mappings.FindPtr(filename)) {
            return *mapping;
        }
        return fm.Mappings.insert(std::make_pair(filename, new TMD5Holder())).first->second;
    }

    void CalculateFileMD5(const TString& filename, const TSharedMD5Holder& md5Holder) {
        with_lock (md5Holder->CalcMutex) {
            if (!md5Holder->Info.MD5 && !md5Holder->Info.Error) {
                try {
                    char md5buf[33];
                    md5Holder->Info.MD5 = MD5::File(filename.data(), md5buf);
                } catch (...) {
                    md5Holder->Info.Error = CurrentExceptionMessage();
                }
            }
        }

        TWriteGuard wg(md5Holder->CalcStateMutex);
        md5Holder->CalcState = CS_Ready;
    }
}

using namespace NFilesMD5Mapping;

void EraseFilenameFromMap(const TString& filename) {
    TFilesMD5Map& fm = GetMappedFiles();
    TWriteGuard fmg(fm.Mutex);
    fm.Mappings.erase(filename);
}

const TMD5Info& TryGetFileMD5FromMap(const TString& filename, IThreadPool& backgroundQueue) {
    TSharedMD5Holder md5Holder = Find(filename);
    {
        TReadGuard rg(md5Holder->CalcStateMutex);
        if (md5Holder->CalcState == CS_Ready) {
            return md5Holder->Info;
        }
    }

    TWriteGuard wg(md5Holder->CalcStateMutex);
    if (md5Holder->CalcState == CS_None) {
        auto calcFunc = [=]() {
            CalculateFileMD5(filename, md5Holder);
        };

        if (backgroundQueue.AddFunc(calcFunc)) {
            md5Holder->CalcState = CS_Scheduled;
        }
    }

    return BUSY;
}

const TMD5Info& GetFileMD5FromMap(const TString& filename) {
    TSharedMD5Holder md5Holder = Find(filename);
    {
        TReadGuard rg(md5Holder->CalcStateMutex);
        if (md5Holder->CalcState == CS_Ready) {
            return md5Holder->Info;
        }
    }

    CalculateFileMD5(filename, md5Holder);
    return md5Holder->Info;
}
