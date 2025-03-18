#pragma once

#include <library/cpp/watchdog/lib/interface.h>
#include <library/cpp/watchdog/timeout/watchdog.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/system/mutex.h>

class TEmergencyCgi;

TTimeoutWatchDogOptions CreateAbortByTimeoutWatchDogOptions(TDuration timeout);

/**
 * Please, pay attention that all data should be passed to watchdog handle via
 * smart pointers. When you pass data by reference or generic pointer,
 * you are in risk, because watchdog handler can be executed AFTER your data
 * was already destroyed (and you got SIGSEGV). See SEARCH-1605 for examples.
 *
 * CreateAbortByTimeoutWatchDog is an exception to rule above. A reference to query
 * via TStringBuf is used. But this watchdog type is used to prevent query
 * processing hang, so when it is fired, query data should be still alive.
 *
 * When timeout is reached CreateTimeoutWatchDog will constantly call callback 
 * until the watchdog destructor is called.
 **/
IWatchDog* CreateTimeoutWatchDog(const TTimeoutWatchDogOptions& options, const std::function<void()>& callback);
IWatchDog* CreateTimeoutWatchDog(const NTimeoutWatchDog::TTimeoutWatchDogConfig& config, const std::function<void()>& callback);
IWatchDog* CreateAbortByTimeoutWatchDog(const TStringBuf query);
IWatchDog* CreateAbortByTimeoutWatchDog(const TTimeoutWatchDogOptions& options, const TStringBuf query);
IWatchDog* CreateAbortByTimeoutWatchDog(const NTimeoutWatchDog::TTimeoutWatchDogConfig& config, const TStringBuf query);
IWatchDog* CreateResourcesWatchDog(size_t maxVirtualMemorySize, int maxFileDescriptors, TDuration delay);
IWatchDog* CreateEmergencyWatchDog(const TString& filename, TDuration frequency, TAtomicSharedPtr<TEmergencyCgi> emergencyCgi);
