#pragma once

#include <library/cpp/safe_stats/safe_stats.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

namespace NSFStats {
    void FromDynamicCounters(const NMonitoring::TDynamicCounters& counters,
        TStats& service, TString labelPath = {}, THashMap<TString, THashSet<TString>> labelsFilter = {});
}
