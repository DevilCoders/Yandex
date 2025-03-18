#pragma once

#include "id_for_string.h"
#include "master_target_type.h"
#include "revision.h"
#include "state_registry.h"

#include <tools/clustermaster/proto/target.pb.h>

#include <time.h>

struct TTimeStats {
    time_t LastStarted;
    time_t LastFinished;
    time_t LastSuccess;
    time_t LastFailure;

    TTimeStats();
    TTimeStats(const TTaskStatus& taskStatus);

    void Update(const NProto::TTaskStatus& taskStatus);
    void Update(const TTaskStatus& taskStatus);
    void Update(const TTimeStats& times);
};

struct TResStats {
    double CpuMax;
    double CpuAvg;
    double MemMax;
    double MemAvg;

    TResStats();
    TResStats(const TTaskStatus& s);
};

struct TDurationStats {
    int Minimum;
    int Maximum;
    int Average;

    TDurationStats();
    TDurationStats(const TTaskStatus& s);
};

struct TAggregateDurationStats {
    int Actual;
    int Total;

    TAggregateDurationStats();
    TAggregateDurationStats(const TTaskStatus& s);
};

struct TRepeatStats {
    int repeatedTimes;
    int maxRepeatTimes;

    TRepeatStats();
    TRepeatStats(const TTaskStatus& s);

    void Update(const TTaskStatus& s);
};

struct TStatsCalculatorResult {
    TResStats Resources;
    TDurationStats Durations;
    TAggregateDurationStats AggregateDurations;
};

struct TStatsCalculator {
    double CpuMax;
    double CpuSum;
    double MemMax;
    double MemSum;

    int Count;

    int DurationMin;
    int DurationMax;
    int DurationSum;

    TVector<std::pair<time_t, time_t> > Times; // pairs of start-of-last-successful and end-of-last-successful
    int TotalDuration;

    bool Finialized;
    TStatsCalculatorResult Result;

    TStatsCalculator();

    void Add(const TTaskStatus& taskStatus);
    void Add(const TResStats& resources, const TDurationStats& duration);
    void Finalize();

    const TResStats& GetResources() { Y_ASSERT(Finialized); return Result.Resources; }
    const TDurationStats& GetDurations() { Y_ASSERT(Finialized); return Result.Durations; }
    const TAggregateDurationStats& GetAggregateDurations() { Y_ASSERT(Finialized); return Result.AggregateDurations; }
};

struct TStateStats {
    TStateCounter Counter;

    TStateStats();
    TStateStats(const TTaskStatus& s);
    TStateStats(const TStateCounter& counter);

    void Update(const TTaskStatus& s);
    void Update(const TStateStats& s);
    /** adds only 1 for every state that has any count greater than 0 */
    void UpdateAndGroup(const TStateStats& s);
};

struct TStats {
    TStateStats Tasks;
    TTimeStats Times;
    TResStats Resources;
    TDurationStats Durations;
    TAggregateDurationStats AggregateDurations;
    TRepeatStats Repeat;

    TStats();
    TStats(const TTaskStatus& s);
};

struct TWorkerStats {
    TWorkerStats()
        : Targets()
        , Stats()
    {
    }

    TWorkerStats(const TStats& stats)
        : Targets()
        , Stats(stats)
    {
    }

    TStateStats Targets;
    TStats Stats;
};

struct TTargetStats {
    TTargetStats()
        : Workers()
        , Stats()
    {
    }

    TTargetStats(const TStats& stats)
        : Workers()
        , Stats(stats)
    {
    }

    TTargetStats(const TStateStats& workers, const TStats& stats)
        : Workers(workers)
        , Stats(stats)
    {
    }

    static TTargetStats FromOnlyOneTask(const TTaskStatus& s) {
        return TTargetStats(TStateStats(s), TStats(s));
    }

    TStateStats Workers;
    TStats Stats;
};

/**
 * Main goal of this class is to speed up web interface by calculating tasks statistics on state
 * update instead of calculating it for each HTTP request.
 */
struct TTargetByWorkerStats {
    typedef TVector<TStats> TVectorType;
    typedef TIdForString::TId THostId;

    TTargetByWorkerStats(const TMasterTargetType& targetType);

    void ChangeState(THostId hostId, const TTaskState& fromState, const TTaskState& toState);
    void UpdateTimes(THostId hostId, const NProto::TTaskStatus& taskStatus);

    TWorkerStats GetStatsForWorker(THostId hostId) const {
        return TWorkerStats(ByWorker.at(hostId));
    }

    TTargetStats GetStatsForTarget() const;
    TTargetStats GetStatsForTargetForWorker(THostId hostId) const {
        return TTargetStats(ByWorker.at(hostId));
    }
    const TStateCounter& GetStateCounter() const;

    TVectorType ByWorker;
    TStats Whole;
    TRevision::TValue RevisionWhenUpdated;
};
