#pragma once

#include "condition.h"
#include "depend.h"
#include "precomputed_task_ids.h"
#include "printer.h"
#include "resource_monitor_resources.h"
#include "target_type_parameters_map.h"

#include <tools/clustermaster/proto/target.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

class TTaskState {
public:
    TTaskState()
        : State(NProto::TS_UNDEFINED), Value(0)
    {}

    TTaskState(const TTaskState&) = default;

    TTaskState(const TTaskState& state, ui64 value)
        : State(state.State), Value(value)
    {}

    TTaskState& operator=(const TTaskState&) = default;

    NProto::ETaskState GetId() const {
        return State;
    }

    ui64 GetValue() const {
        return Value;
    }

    bool Equal(const TTaskState& s) const {
        return State == s.State && Value == s.Value;
    }

    bool operator == (const TTaskState& s) const {
        return State == s.State;
    }

    bool operator != (const TTaskState& s) const {
        return State != s.State;
    }

    static TTaskState Make(NProto::ETaskState state = NProto::TS_UNDEFINED, ui64 value = 0) {
        return TTaskState(state, value);
    }

private:
    TTaskState(NProto::ETaskState state, ui64 value)
        : State(state), Value(value)
    {}

private:
    NProto::ETaskState State;
    ui64 Value;
};

static const TTaskState TS_UNDEFINED  = TTaskState::Make(NProto::TS_UNDEFINED);
static const TTaskState TS_IDLE       = TTaskState::Make(NProto::TS_IDLE);
static const TTaskState TS_PENDING    = TTaskState::Make(NProto::TS_PENDING);
static const TTaskState TS_READY      = TTaskState::Make(NProto::TS_READY);
static const TTaskState TS_RUNNING    = TTaskState::Make(NProto::TS_RUNNING);
static const TTaskState TS_STOPPING   = TTaskState::Make(NProto::TS_STOPPING);
static const TTaskState TS_STOPPED    = TTaskState::Make(NProto::TS_STOPPED);
static const TTaskState TS_CONTINUING = TTaskState::Make(NProto::TS_CONTINUING);
static const TTaskState TS_SUSPENDED  = TTaskState::Make(NProto::TS_SUSPENDED);
static const TTaskState TS_SUCCESS    = TTaskState::Make(NProto::TS_SUCCESS);
static const TTaskState TS_SKIPPED    = TTaskState::Make(NProto::TS_SKIPPED);
static const TTaskState TS_FAILED     = TTaskState::Make(NProto::TS_FAILED);
static const TTaskState TS_DEPFAILED  = TTaskState::Make(NProto::TS_DEPFAILED);
static const TTaskState TS_CANCELING  = TTaskState::Make(NProto::TS_CANCELING);
static const TTaskState TS_CANCELED   = TTaskState::Make(NProto::TS_CANCELED);
static const TTaskState TS_UNKNOWN    = TTaskState::Make(NProto::TS_UNKNOWN);


template <typename PTypes>
class TTargetGraphBase;

template <typename PTypes>
struct TTargetGraphParserState;

struct TTaskStatus {
    TTaskStatus()
        : State(TS_IDLE)
        , LastStarted(0)
        , LastFinished(0)
        , LastSuccess(0)
        , LastFailure(0)
        , LastDuration(0)
        , LastChanged(0)
        , StartedCounter(0)
        , DoNotTrackMarker(false)
        , toRepeat(0)
        , RepeatMax(0)
        , Pid(-1)
        , LastProcStarted(0)
    {
        resUsageStat = defaultResUsage;
    }

    // XXX: protobuf-mimicking accessors/mutators
    inline const TTaskState& GetState() const { return State; }
    inline bool IsStopped() const { return GetState() == TS_STOPPING || GetState() == TS_STOPPED || GetState() == TS_CONTINUING; }
    inline ui32 GetLastStarted() const { return LastStarted; }
    inline ui32 GetLastFinished() const { return LastFinished; }
    inline ui32 GetLastSuccess() const { return LastSuccess; }
    inline ui32 GetLastFailure() const { return LastFailure; }
    inline i32  GetLastDuration() const { return LastDuration; }
    inline ui32 GetLastChanged() const { return LastChanged; }
    inline ui32 GetStartedCounter() const { return StartedCounter; }
    inline bool DoNotTrack() const { return DoNotTrackMarker; }
    inline ui32 GetToRepeat() const { return toRepeat; }
    inline ui32 GetRepeatMax() const {  return RepeatMax; }

    inline double GetCpuMax() const { return resUsageStat.cpuMax; }
    inline double GetCpuMaxDelta() const { return resUsageStat.cpuMaxDelta; }
    inline double GetCpuAvg() const { return resUsageStat.cpuAvg; }
    inline double GetCpuAvgDelta() const { return resUsageStat.cpuAvgDelta; }
    inline double GetMemMax() const { return resUsageStat.memMax; }
    inline double GetMemMaxDelta() const { return resUsageStat.memMaxDelta; }
    inline double GetMemAvg() const { return resUsageStat.memAvg; }
    inline double GetMemAvgDelta() const { return resUsageStat.memAvgDelta; }
    inline double GetIOMax() const { return resUsageStat.ioMax; }
    inline double GetIOMaxDelta() const { return resUsageStat.ioMaxDelta; }
    inline double GetIOAvg() const { return resUsageStat.ioAvg; }
    inline double GetIOAvgDelta() const { return resUsageStat.ioAvgDelta; }
    inline ui32   GetUpdateCounter() const { return resUsageStat.updateCounter; }

    inline const resUsageStat_st& GetResStat() const { return resUsageStat; }

    inline i32 GetPid() const { return Pid; }
    inline ui32 GetLastProcStarted() const { return LastProcStarted; }

    inline void SetState(const TTaskState& s)        { State = s; }
    inline void SetLastStarted(ui32 s)  { LastStarted = s; }
    inline void SetLastFinished(ui32 s) { LastFinished = s; }
    inline void SetLastSuccess(ui32 s)  { LastSuccess = s; }
    inline void SetLastFailure(ui32 s)  { LastFailure = s; }
    inline void SetLastDuration(i32 s)  { LastDuration = s; }
    inline void SetLastChanged(ui32 s)  { LastChanged = s; }
    inline void SetStartedCounter(ui32 s){StartedCounter = s; }
    inline void ToggleDoNotTtackMarker(bool s)  { DoNotTrackMarker = s; }
    inline void SetToRepeat(ui32 s)     { toRepeat = s; }
    inline void SetRepeatMax(ui32 s)    { RepeatMax = s; }
    inline void IncStartedCounter()     { ++StartedCounter; }
    inline void SetPid(i32 s)           { Pid = s; }
    inline void SetLastProcStarted(ui32 s) { LastProcStarted = s; }

    inline void SetCpuMax(double s)       {  resUsageStat.cpuMax = s; }
    inline void SetCpuMaxDelta(double s)  {  resUsageStat.cpuMaxDelta = s; }
    inline void SetCpuAvg(double s)       {  resUsageStat.cpuAvg = s; }
    inline void SetCpuAvgDelta(double s)  {  resUsageStat.cpuAvgDelta = s; }
    inline void SetMemMax(double s)       {  resUsageStat.memMax = s; }
    inline void SetMemMaxDelta(double s)  {  resUsageStat.memMaxDelta = s; }
    inline void SetMemAvg(double s)       {  resUsageStat.memAvg = s; }
    inline void SetMemAvgDelta(double s)  {  resUsageStat.memAvgDelta = s; }
    inline void SetIOMax(double s)        {  resUsageStat.ioMax = s; }
    inline void SetIOMaxDelta(double s)   {  resUsageStat.ioMaxDelta = s; }
    inline void SetIOAvg(double s)        {  resUsageStat.ioAvg = s; }
    inline void SetIOAvgDelta(double s)   {  resUsageStat.ioAvgDelta = s; }
    inline void SetUpdateCounter(ui32 s)  {  resUsageStat.updateCounter = s; }
    inline void IncUpdateCounter()        {  resUsageStat.updateCounter++; }

    inline void ResetResourceStatistics() {
        resUsageStatHistory.clear();
        resUsageStat.updateCounter = 0;

        updateCpuStat();
        updateMemStat();
    }

    inline void AddSharedResDefinition(TString const& Name, double const Val) {
        if (!resUsageStat.sharedAreas) {
            resUsageStat.sharedAreas = new TMap<TString, double>;
        }
        (*resUsageStat.sharedAreas)[Name] = Val;
    }

    inline void updateCpuStat() {
        double cpuMax = 0.00;
        double cpuAvg = 0.00;

        if (resUsageStatHistory.size()) {
            for (TList<resUsageStat_st>::iterator it = resUsageStatHistory.begin();
                    it != resUsageStatHistory.end(); ++it) {
                cpuAvg += (*it).cpuAvg;
                cpuMax = Max (cpuMax, (*it).cpuMax);
            }
            resUsageStat.cpuMax = cpuMax;
            resUsageStat.cpuAvg = cpuAvg/resUsageStatHistory.size();
        } else {
            resUsageStat.cpuMax = defaultResUsage.cpuMax;
            resUsageStat.cpuAvg = defaultResUsage.cpuAvg;
        }
        /* ToDo: Посчитать среднеквадратичное отклонение */
    }

    inline void updateMemStat() {
        double memMax = 0.00;
        double memAvg = 0.00;

        if (resUsageStatHistory.size()) {
            for (TList<resUsageStat_st>::iterator it = resUsageStatHistory.begin();
                    it != resUsageStatHistory.end(); ++it) {
                memAvg += (*it).memAvg;
                memMax = Max(memMax, (*it).memMax);
            }

            resUsageStat.memMax = memMax;
            resUsageStat.memAvg = memAvg/resUsageStatHistory.size();
        } else {
            resUsageStat.memMax = defaultResUsage.memMax;
            resUsageStat.memAvg = defaultResUsage.memAvg;
        }

        /* ToDo: Посчитать среднеквадратичное отклонение */
    }

    inline void updateCpuMax(double s) {
        resUsageStat.cpuMax = resUsageStat.updateCounter ? Max<double>(resUsageStat.cpuMax, s) : s;
    }
    inline void updateMemMax(double s) {
        resUsageStat.memMax = resUsageStat.updateCounter ? Max<double>(resUsageStat.memMax, s) : s;
    }
    inline void updateIOMax(double s) {
        resUsageStat.ioMax = resUsageStat.updateCounter ? Max<double>(resUsageStat.ioMax, s) : s;
    }
    inline void updateCpuAvg(double s) {
        resUsageStat.cpuAvg = (resUsageStat.cpuAvg * resUsageStat.updateCounter + s) / (resUsageStat.updateCounter + 1);
    }
    inline void updateMemAvg(double s) {
        resUsageStat.memAvg = (resUsageStat.memAvg * resUsageStat.updateCounter + s) / (resUsageStat.updateCounter + 1);
    }
    inline void updateIOAvg(double s) {
        resUsageStat.ioAvg = (resUsageStat.ioAvg * resUsageStat.updateCounter + s) / (resUsageStat.updateCounter + 1);
    }
    inline void updateAllResStat(const resUsageStat_st & newStatistics) {
        /*
         * ToDo: Calculate variance of this values
         */
        resUsageStat.sharedAreas = newStatistics.sharedAreas;
        const_cast<resUsageStat_st *>(&newStatistics)->sharedAreas = nullptr;
        resUsageStatHistory.push_back(newStatistics);
        if (resUsageStatHistory.size() > MAX_STATISTICS_NUMBER) {
            resUsageStatHistory.pop_front();
        }

        updateCpuStat();
        updateMemStat();

        IncUpdateCounter();
    }

    TTaskStatus &operator=(const NProto::TTaskStatus& right) {
        State = TTaskState::Make(right.GetState(), right.GetStateValue());
        LastStarted = right.GetLastStarted();
        LastFinished = right.GetLastFinished();
        LastSuccess = right.GetLastSuccess();
        LastFailure = right.GetLastFailure();
        LastDuration = right.GetLastDuration();
        LastChanged = right.GetLastChanged();
        StartedCounter = right.GetStartedCounter();
        toRepeat = right.GetToRepeat();
        RepeatMax = right.GetRepeatMax();
        Pid = right.GetPid();
        LastProcStarted = right.GetLastProcStarted();

        resUsageStat.cpuMax = right.GetCpuMax();
        resUsageStat.cpuMaxDelta = right.GetCpuMaxDelta();
        resUsageStat.cpuAvg = right.GetCpuAvg();
        resUsageStat.cpuAvgDelta = right.GetCpuAvgDelta();
        resUsageStat.memMax = right.GetMemMax();
        resUsageStat.memMaxDelta = right.GetMemMaxDelta();
        resUsageStat.memAvg = right.GetMemAvg();
        resUsageStat.memAvgDelta = right.GetMemAvgDelta();
        resUsageStat.ioMax = right.GetIOMax();
        resUsageStat.ioMaxDelta = right.GetIOMaxDelta();
        resUsageStat.ioAvg = right.GetIOAvg();
        resUsageStat.ioAvgDelta = right.GetIOAvgDelta();
        resUsageStat.updateCounter = right.GetUpdateCounter();

        return *this;
    }

private:
    TTaskState State;
    ui32 LastStarted;
    ui32 LastFinished;
    ui32 LastSuccess;
    ui32 LastFailure;
    i32  LastDuration;
    ui32 LastChanged;
    ui32 StartedCounter;
    bool DoNotTrackMarker;
    /*
     * Number of times this tasks has been run. It's have sense only for tasks included in cyclic path.
     * Such tasks can be run several times in workflow.
     * But some tasks wants to know it's value, so we shouldn't reset it in INVALIDATE task.
     */
    ui32 toRepeat;
    ui32 RepeatMax;
    i32  Pid;
    ui32 LastProcStarted;

    resUsageStat_st resUsageStat;
    TList<resUsageStat_st> resUsageStatHistory;

    static const ui8 MAX_STATISTICS_NUMBER = 9;
};

class TStateRegistry {
public:
    struct TStateInfo {
        TTaskState State;
        TString SmallName;
        TString XmlTag;
        TString CapName;
        TString JsonId;
    };

public:
    typedef const TStateInfo* const_iterator;

public:
    static size_t GetStatesCount();

    static const_iterator begin();
    static const_iterator end();

    static size_t find(const TTaskState& s);
    static const TStateInfo* findByState(const TTaskState& s);
    static const TStateInfo* findBySmallName(const TString& s);
};

template <>
inline TString ToString<TTaskState>(const TTaskState& s) {
    return TStateRegistry::findByState(s)->SmallName;
}

template <>
inline TTaskState FromString<TTaskState>(const TString& s) {
    return TStateRegistry::findBySmallName(s)->State;
}

class TTaskStatuses: public NProto::TTaskStatuses {
public:
    TTaskStatuses() {
    }

    ~TTaskStatuses() override {
    }

    typedef google::protobuf::RepeatedPtrField<NProto::TTaskStatus> TRepeatedStatus;
    typedef google::protobuf::RepeatedPtrField<NProto::TTaskStatus::TSharedDefinition> TRepeatedSharedRes;
};

template <typename PTypes>
class TTargetBase {
protected:
    typedef typename PTypes::TTargetType TThisTargetType;
    typedef typename PTypes::TGraph TGraph;
    typedef typename PTypes::TTarget TThis;
    typedef typename PTypes::TDepend TDepend;
    typedef typename PTypes::TSpecificTaskStatus TSpecificTaskStatus;
    typedef TTargetGraphParserState<PTypes> TParserState;
public:

    class TTraversalGuard: private THashSet<size_t>, TNonCopyable {
    public:
        TTraversalGuard()
            : THashSet<size_t>(1000)
        {
        }
        bool TryVisit(const typename PTypes::TTarget* target) {
            return insert(reinterpret_cast<size_t>(target)).second;
        }
    };

    const TString Name;
    typename PTypes::TTargetType* const    Type;

    typedef TVector<TDepend> TDependsList;

    TDependsList Depends;   // Targets this target depends on
    TDependsList Followers; // Targets depending on this target

    // in master filled by analyzing depends
    // in worker filled received from master
    bool HasCrossnodeDepends;


    typedef TTargetTypeParametersMap<TSpecificTaskStatus> TTaskList;
    TTaskList Tasks;


    TTargetBase(const TString& n, TThisTargetType* t, const TParserState&)
        : Name(n)
        , Type(t)
        , HasCrossnodeDepends(false)
        , Tasks(&t->GetParameters())
    {
    }

    virtual ~TTargetBase() {}

    const TString& GetName() const { return Name; }

    const TTaskList& GetTasks() const noexcept { return Tasks; }

    TTaskList& GetTasks() noexcept { return Tasks; }

    // TODO: make const
    virtual void RegisterDependency(TThis* depend, TTargetParsed::TDepend& dependParsed, const typename TTargetGraphBase<PTypes>::TParserState&,
            typename PTypes::TPrecomputedTaskIdsContainer*)
    {
        // Disallow duplicate depends
        for (typename TDependsList::const_iterator i = Depends.begin(); i != Depends.end(); ++i) {
            if (i->GetTarget() == depend) {
                ythrow yexception() << "duplicate dependency '" << depend->Name << "' for target '" << Name << "'";
            }
        }

        Depends.push_back(TDepend(reinterpret_cast<TThis*>(this), depend, false, dependParsed));
        depend->Followers.push_back(TDepend(depend, reinterpret_cast<TThis*>(this), true, dependParsed));
    }

    virtual bool IsSame(const TThis* ) const {
        return false;
    }

    virtual void SwapState(TThis* ) {
    }

    virtual void LoadState() {
    }

    virtual void SaveState() const {
    }

    virtual void AddOption(const TString&, const TString&) {
    }

    class TTaskIterator;

    class TConstTaskIterator {
        friend class TTaskIterator;
    protected:
        const TTargetBase* const Target;
        TTargetTypeParameters::TIterator ParametersIterator;
    public:
        TConstTaskIterator(const TTargetBase* target)
            : Target(target)
            , ParametersIterator(target->Type->GetParameters().Iterator())
        { }

        TConstTaskIterator(const TTargetBase* target,
                const TTargetTypeParameters::TProjection& filter)
            : Target(target)
            , ParametersIterator(target->Type->GetParameters().Iterator(filter))
        { }

        bool Next();

        const TSpecificTaskStatus& operator*() const;

        const TSpecificTaskStatus* operator->() const;

        TString GetTaskName() const;

        TTargetTypeParameters::TPath GetPath() const;

        TTargetTypeParameters::TId GetN() const;

        const TTargetTypeParameters::TIdPath& GetPathId() const;
    };

    // TODO there should be no separate TFlatConstTaskIterator class; move new logic into 'ordinary' TTaskIterator
    class TFlatConstTaskIterator {
        friend class TTargetBase;
    private:
        const TTargetBase* const Target;
        const ui32 TasksCount;
        const TMaybe<TTargetTypeParameters::TParamId> HostId;
        ui32 Current;
    public:
        TFlatConstTaskIterator(const TTargetBase* target, TMaybe<TTargetTypeParameters::TParamId> hostId)
            : Target(target)
            , TasksCount(target->Type->GetParameters().GetCount())
            , HostId(hostId) // TODO 'flat' iterator with hostId specified is too slow (see speed_test.cpp)
            , Current(0)
        {}

        bool Next();

        const TSpecificTaskStatus& operator*() const;

        const TSpecificTaskStatus* operator->() const;

        TTargetTypeParameters::TPath GetPath() const;

        TTargetTypeParameters::TId GetN() const;
    };

    class TTaskIterator: public TConstTaskIterator {
        friend class TTargetBase;
    public:
        TTaskIterator(TTargetBase* target)
            : TConstTaskIterator(target)
        { }

        TTaskIterator(
                TTargetBase* target,
                const TTargetTypeParameters::TProjection& filter)
            : TConstTaskIterator(target, filter)
        { }

    public:
        TSpecificTaskStatus& operator*() const {
            return const_cast<TSpecificTaskStatus&>(TConstTaskIterator::operator*());
        }

        TSpecificTaskStatus* operator->() const {
            return const_cast<TSpecificTaskStatus*>(TConstTaskIterator::operator->());
        }
    };


    TTaskIterator TaskIteratorForProjection(const TTargetTypeParameters::TProjection& proj) {
        return TTaskIterator(this, proj);
    }

    TConstTaskIterator TaskIteratorForProjection(const TTargetTypeParameters::TProjection& proj) const {
        return TConstTaskIterator(this, proj);
    }

    TTaskIterator TaskIterator() {
        return TaskIteratorForProjection(TTargetTypeParameters::TProjection::True(Type->GetParameters().GetDepth()));
    }

    TConstTaskIterator TaskIterator() const {
        return TaskIteratorForProjection(TTargetTypeParameters::TProjection::True(Type->GetParameters().GetDepth()));
    }

    TFlatConstTaskIterator TaskFlatIterator() const {
        return TFlatConstTaskIterator(this, TMaybe<TTargetTypeParameters::TParamId>());
    }

    TTaskIterator TaskIteratorForLevel(TLevelId levelId, const TIdForString::TIdSafe taskId) {
        return TaskIteratorForProjection(
                TTargetTypeParameters::TProjection::SingleParam(
                        levelId, taskId, Type->GetParameters().GetDepth()));
    }

    TConstTaskIterator TaskIteratorForLevel(TLevelId levelId, const TIdForString::TIdSafe taskId) const {
        return TaskIteratorForProjection(
                TTargetTypeParameters::TProjection::SingleParam(
                        levelId, taskId, Type->GetParameters().GetDepth()));
    }

    TTaskIterator TaskIteratorForSingleTask(const TTargetTypeParameters::TId& n) {
        return TaskIteratorForProjection(Type->GetParameters().NProjection(n));
    }

    TConstTaskIterator TaskIteratorForSingleTask(const TTargetTypeParameters::TId& n) const {
        return TaskIteratorForProjection(Type->GetParameters().NProjection(n));
    }


    virtual void CheckState() const {
        Y_VERIFY(Tasks.GetParameters() == &Type->GetParameters(), "parameters is different in Tasks and Type");
    }

    void DumpState(TPrinter& out) const {
        out.Println(TString() + "Target " + Name + " of type " + Type->GetName() + ":");

        TPrinter l1 = out.Next();

        if (HasCrossnodeDepends) {
            l1.Println("has crossnode depends");
        } else {
            l1.Println("has no crossnode depends");
        }

        l1.Println("Depends:");
        for (typename TDependsList::const_iterator depend = Depends.begin();
                depend != Depends.end(); ++depend)
        {
            TPrinter l2 = l1.Next();
            depend->DumpState(l2);
        }
        l1.Println("Depends.");

        DumpStateExtra(l1);

        out.Println(TString() + "Target " + Name + ".");
    }

    virtual void DumpStateExtra(TPrinter&) const {}

    /* if useDepends is true then method uses Depends otherwise it uses Followers */
    template <typename PTargetEdgePredicate>
    void TopoSortedSubgraph(TList<const TThis*>* result, bool useDepends, TTraversalGuard& guard) const {
        const TThis* thiz = reinterpret_cast<const TThis*>(this);

        if (!guard.TryVisit(thiz))
            return;
        if (!PTargetEdgePredicate::TargetIsOk(*dynamic_cast<const typename PTypes::TTarget*>(this)))
            return;

        const TDependsList& edges = useDepends ? Depends : Followers;
        for (typename TDependsList::const_iterator edge = edges.begin();
                edge != edges.end(); ++edge)
        {
            if (!PTargetEdgePredicate::EdgeIsOk(*edge))
                continue;
            edge->GetTarget()->template TopoSortedSubgraph<PTargetEdgePredicate>(result, useDepends, guard);
        }

        result->push_back(thiz);
    }

};
