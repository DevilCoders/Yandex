#include "watchdog.h"

#include <library/cpp/watchdog/emergency/watchdog.h>
#include <library/cpp/watchdog/lib/handle.h>
#include <library/cpp/watchdog/lib/factory.h>
#include <library/cpp/watchdog/resources/watchdog.h>
#include <library/cpp/watchdog/timeout/watchdog.h>

#include <library/cpp/json/json_reader.h>

#include <util/folder/dirut.h>
#include <util/generic/algorithm.h>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/system/event.h>
#include <util/system/maxlen.h>
#include <util/thread/factory.h>
#include <util/thread/lfqueue.h>

#include <iostream>
#include <thread>

namespace {
    void LogQueryAndAbort(TStringBuf query, const std::thread::id sourceThreadId) {
        std::cerr << std::hex << std::showbase;
        std::cerr << "Watchdog fired for query:" << std::endl;
        std::cerr << query << std::endl;
        std::cerr << "Wathdog source thread id is:" << std::endl;
        std::cerr << sourceThreadId << std::endl;
        Y_FAIL();
    }
}

TTimeoutWatchDogOptions CreateAbortByTimeoutWatchDogOptions(TDuration timeout) {
    return TTimeoutWatchDogOptions(timeout)
        .SetRandomTimeoutSpread(timeout / 2);
}

// Include files for TResourcesWatchDogHandle
IWatchDog* CreateTimeoutWatchDog(const TTimeoutWatchDogOptions& options, const std::function<void()>& callback) {
    return TTimeoutWatchDogHandle::Create(options, callback);
}

IWatchDog* CreateTimeoutWatchDog(const NTimeoutWatchDog::TTimeoutWatchDogConfig& config, const std::function<void()>& callback) {
    return CreateTimeoutWatchDog(TTimeoutWatchDogOptions(config), callback);
}

IWatchDog* CreateAbortByTimeoutWatchDog(const TStringBuf query) {
    return CreateAbortByTimeoutWatchDog(CreateAbortByTimeoutWatchDogOptions(TDuration::Minutes(1)), query);
}

IWatchDog* CreateAbortByTimeoutWatchDog(const TTimeoutWatchDogOptions& options, const TStringBuf query) {
    return TTimeoutWatchDogHandle::Create(options,
        [query, sourceThreadId = std::this_thread::get_id()]() {
            LogQueryAndAbort(query, sourceThreadId);
        }
    );
}

IWatchDog* CreateAbortByTimeoutWatchDog(const NTimeoutWatchDog::TTimeoutWatchDogConfig& config, const TStringBuf query) {
    return CreateAbortByTimeoutWatchDog(TTimeoutWatchDogOptions(config), query);
}

IWatchDog* CreateResourcesWatchDog(size_t maxVirtualMemorySize, int maxFileDescriptors, TDuration delay) {
    return TResourcesWatchDogHandle::Create(maxVirtualMemorySize, maxFileDescriptors, delay);
}

IWatchDog* CreateEmergencyWatchDog(const TString& filename, TDuration frequency, TAtomicSharedPtr<TEmergencyCgi> emergencyCgi) {
    return TEmergencyWatchDogHandle::Create(filename, frequency, emergencyCgi);
}
