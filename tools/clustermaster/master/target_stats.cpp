#include "target_stats.h"

TTimeStats::TTimeStats()
    : LastStarted(0)
    , LastFinished(0)
    , LastSuccess(0)
    , LastFailure(0)
{
}

TTimeStats::TTimeStats(const TTaskStatus& s)
    : LastStarted(s.GetLastStarted())
    , LastFinished(s.GetLastFinished())
    , LastSuccess(s.GetLastSuccess())
    , LastFailure(s.GetLastFailure())
{
}

template <class T>
void TTimeStatsUpdate(TTimeStats* thiz, const T& status) {
    thiz->LastStarted = Max<time_t>(thiz->LastStarted, status.GetLastStarted());
    thiz->LastFinished = Max<time_t>(thiz->LastFinished, status.GetLastFinished());
    thiz->LastSuccess = Max<time_t>(thiz->LastSuccess, status.GetLastSuccess());
    thiz->LastFailure = Max<time_t>(thiz->LastFailure, status.GetLastFailure());
}

void TTimeStats::Update(const NProto::TTaskStatus& taskStatus) {
    TTimeStatsUpdate<NProto::TTaskStatus>(this, taskStatus);
}

void TTimeStats::Update(const TTaskStatus& taskStatus) {
    TTimeStatsUpdate<TTaskStatus>(this, taskStatus);
}

void TTimeStats::Update(const TTimeStats& times) {
    LastStarted = Max<time_t>(LastStarted, times.LastStarted);
    LastFinished = Max<time_t>(LastFinished, times.LastFinished);
    LastSuccess = Max<time_t>(LastSuccess, times.LastSuccess);
    LastFailure = Max<time_t>(LastFailure, times.LastFailure);
}

TResStats::TResStats()
    : CpuMax(0)
    , CpuAvg(0)
    , MemMax(0)
    , MemAvg(0)
{
}

TResStats::TResStats(const TTaskStatus& s)
    : CpuMax(s.GetCpuMax())
    , CpuAvg(s.GetCpuAvg())
    , MemMax(s.GetMemMax())
    , MemAvg(s.GetMemAvg())
{
}

TDurationStats::TDurationStats()
    : Minimum(0)
    , Maximum(0)
    , Average(0)
{
}

TDurationStats::TDurationStats(const TTaskStatus& s)
    : Minimum(s.GetLastDuration())
    , Maximum(s.GetLastDuration())
    , Average(s.GetLastDuration())
{
}

TAggregateDurationStats::TAggregateDurationStats()
    : Actual(0)
    , Total(0)
{
}

TAggregateDurationStats::TAggregateDurationStats(const TTaskStatus& s)
    : Actual(s.GetLastDuration())
    , Total(s.GetLastDuration())
{
}

TRepeatStats::TRepeatStats()
    : repeatedTimes(0)
    , maxRepeatTimes(0)
{}

TRepeatStats::TRepeatStats(const TTaskStatus& s)
    : repeatedTimes(s.GetToRepeat())
    , maxRepeatTimes(s.GetRepeatMax())
{}

void TRepeatStats::Update(const TTaskStatus& s) {
    repeatedTimes = s.GetToRepeat();
    maxRepeatTimes = s.GetRepeatMax();
}

TStatsCalculator::TStatsCalculator()
    : CpuMax(0)
    , CpuSum(0)
    , MemMax(0)
    , MemSum(0)
    , Count(0)
    , DurationMin(-1)
    , DurationMax(0)
    , DurationSum(0)
    , TotalDuration(0)
    , Finialized(false)
    , Result()
{
}

void TStatsCalculator::Add(const TTaskStatus& taskStatus) {
    Count++;

    CpuMax = Max<double>(CpuMax, taskStatus.GetCpuMax());
    CpuSum += taskStatus.GetCpuAvg();
    MemMax = Max<double>(MemMax, taskStatus.GetMemMax());
    MemSum += taskStatus.GetMemAvg();

    DurationMax = Max<int>(DurationMax, taskStatus.GetLastDuration());
    if (taskStatus.GetLastDuration() < DurationMin || DurationMin < 0) {
        DurationMin = taskStatus.GetLastDuration();
    }
    DurationSum += taskStatus.GetLastDuration();

    if (taskStatus.GetLastDuration() >= 0 && taskStatus.GetLastSuccess() > 0 && taskStatus.GetLastSuccess() >= unsigned(taskStatus.GetLastDuration())) {
        time_t startOfLastSuccessful = taskStatus.GetLastSuccess() - taskStatus.GetLastDuration();
        time_t endOfLastSuccessful = taskStatus.GetLastSuccess();
        Times.push_back(std::make_pair(startOfLastSuccessful, endOfLastSuccessful));
        TotalDuration += taskStatus.GetLastDuration();
    }
}

void TStatsCalculator::Add(const TResStats& resources, const TDurationStats& duration) {
    Count++;

    CpuMax = Max<double>(CpuMax, resources.CpuMax);
    CpuSum += resources.CpuAvg;
    MemMax = Max<double>(MemMax, resources.MemMax);
    MemSum += resources.MemAvg;

    DurationMax = Max<int>(DurationMax, duration.Maximum);
    if (duration.Minimum < DurationMin || DurationMin < 0) {
        DurationMin = duration.Minimum;
    }
    DurationSum += duration.Average;

    // there is no way (and no need - see usages of this method) to prepare data for Result.AggregateDurations
}

void TStatsCalculator::Finalize() {
    if (Count > 0) {
        Result.Durations.Maximum = DurationMax;
        Result.Durations.Minimum = DurationMin;
        Result.Durations.Average = DurationSum / Count;

        Result.Resources.CpuMax = CpuMax;
        Result.Resources.CpuAvg = CpuSum / Count;
        Result.Resources.MemMax = MemMax;
        Result.Resources.MemAvg = MemSum / Count;

        // To understand code below you should know that Result.AggregateDurations.Actual is duration of whole
        // target (or some subset of target's tasks) calculated with respect of the fact that tasks are run in parallel
        if (!Times.empty()) {
            Sort(Times.begin(), Times.end());

            std::pair<time_t, time_t> continuous(*Times.begin());
            for (TVector<std::pair<time_t, time_t> >::const_iterator i = Times.begin() + 1; i != Times.end(); ++i) {
                if (i->first > continuous.second) { // if gap then
                    Result.AggregateDurations.Actual += continuous.second - continuous.first; // save duration of old continuous piece
                    continuous = *i; // and set new continuous
                } else { // else
                    continuous.second = Max(continuous.second, i->second); // increase continuous
                }
            }
            Result.AggregateDurations.Actual += continuous.second - continuous.first; // last continuous
            Result.AggregateDurations.Total = TotalDuration;
        } else {
            Result.AggregateDurations.Actual = -1;
            Result.AggregateDurations.Total = -1;
        }
    } else {
        Result.AggregateDurations.Actual = -1;
        Result.AggregateDurations.Total = -1;
        // other fields have 0 values from default constructors
    }
    Finialized = true;
}

TStateStats::TStateStats()
{
}

TStateStats::TStateStats(const TTaskStatus& s)
    : Counter(1, s.GetState())
{
}

TStateStats::TStateStats(const TStateCounter& counter)
    : Counter(counter)
{
}

void TStateStats::Update(const TTaskStatus& s) {
    Counter.AddStates(1, s.GetState());
}

void TStateStats::Update(const TStateStats& s) {
    for (TStateRegistry::const_iterator i = TStateRegistry::begin(); i != TStateRegistry::end(); ++i) {
        int count = s.Counter.GetCount(i->State);
        if (count > 0)
            Counter.AddStates(count, i->State);
    }
}

/** adds only 1 for every state that has any count greater than 0 */
void TStateStats::UpdateAndGroup(const TStateStats& s) {
    for (TStateRegistry::const_iterator i = TStateRegistry::begin(); i != TStateRegistry::end(); ++i) {
        if (s.Counter.GetCount(i->State))
            Counter.AddStates(1, i->State);
    }
}

TStats::TStats()
    : Tasks()
    , Times()
    , Resources()
    , Durations()
    , AggregateDurations()
    , Repeat()
{
}

TStats::TStats(const TTaskStatus& s)
    : Tasks(s)
    , Times(s)
    , Resources(s)
    , Durations(s)
    , AggregateDurations(s)
    , Repeat(s)
{
}

TTargetByWorkerStats::TTargetByWorkerStats(const TMasterTargetType& targetType)
    : ByWorker(targetType.GetHostCount())
    , Whole()
    , RevisionWhenUpdated(0)
{
    for (TIdForString::TIterator host = targetType.GetHostList().Iterator(); host.Next(); ) {
        ByWorker.at(host->Id).Tasks.Counter.AddStates(targetType.GetTaskCountByWorker(*host), TS_UNKNOWN);
    }
    Whole.Tasks.Counter.AddStates(targetType.GetParameters().GetCount(), TS_UNKNOWN);
}

void TTargetByWorkerStats::ChangeState(THostId hostId, const TTaskState& fromState, const TTaskState& toState) {
    ByWorker.at(hostId).Tasks.Counter.ChangeState(fromState, toState);
    Whole.Tasks.Counter.ChangeState(fromState, toState);
}

void TTargetByWorkerStats::UpdateTimes(THostId hostId, const NProto::TTaskStatus& taskStatus) {
    ByWorker.at(hostId).Times.Update(taskStatus);
    Whole.Times.Update(taskStatus);
}

TTargetStats TTargetByWorkerStats::GetStatsForTarget() const {
    TTargetStats result(Whole);
    for (TVectorType::const_iterator w = ByWorker.begin(); w != ByWorker.end(); w++) {
        result.Workers.UpdateAndGroup(w->Tasks);
    }
    return result;
}

const TStateCounter& TTargetByWorkerStats::GetStateCounter() const {
    return Whole.Tasks.Counter;
}
