#pragma once

#include "cron_state.h"
#include "lockablehandle.h"
#include "master_list_manager.h"
#include "master_target_graph_types.h"
#include "master_target_type.h"
#include "master_variables.h"
#include "notification.h"
#include "precomputed_task_ids_group.h"
#include "recipients.h"
#include "revision.h"
#include "state_registry.h"
#include "target_stats.h"
#include "type_if_else.h"

#include <tools/clustermaster/common/cron.h>
#include <tools/clustermaster/common/printer.h>
#include <tools/clustermaster/common/target_impl.h>
#include <tools/clustermaster/common/target_type_parameters_map.h>

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>

enum class ECronState {
    Failed = 0,
    Succeeded,
};

template<>
TString ToString<ECronState>(const ECronState& s);

class TMasterGraph;
class TWorkerPool;

struct TPokeMessage;

struct TMasterPrecomputedTaskIdsContainer {
    TPrecomputedTaskIdsContainer<TMasterGraphTypes, TParamsByTypeOrdinary> Container;
    TMasterPrecomputedTaskIdsContainerGroup ContainerGroup;
};

struct TMasterTaskStatus {
    TMasterTaskStatus()
        : Status()
        , PokeReady(false)
        , PokeDepfail(false) {}

    TTaskStatus Status;
    bool PokeReady;
    bool PokeDepfail;
};

class TMasterTarget: public TTargetBase<TMasterGraphTypes>, TNonCopyable {
    friend class TMasterGraph;

public:
    typedef TMasterTargetType::THostId THostId;
    typedef TMasterTargetType::THostIdSafe THostIdSafe;
    typedef TMasterTargetType::TParamValueIdUnsafe TGlobalTaskId;
    typedef TMasterTargetType::TWorkerLocalTaskId TWorkerLocalTaskId;
    typedef TMasterTargetType::TParamValueId TTaskIdSafe;
    typedef TMasterTargetType::TParamValueIdUnsafes TTaskIdList;
    typedef TMasterTargetType::TTaskId TTaskId;

public:
    TMasterTarget(const TString& n, TMasterTargetType* t, const TParserState&);
    ~TMasterTarget() override {}

    void RegisterDependency(TMasterTarget* depend, TTargetParsed::TDepend&, const TParserState&,
            TMasterGraphTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer) override;
    void AddOption(const TString& key, const TString& value) override;

    bool IsSame(const TMasterTarget* right) const override;
    void SwapState(TMasterTarget* right) override;
    TMaybe<NProto::TMasterTargetState> LoadStateFromYt() const;
    TMaybe<NProto::TMasterTargetState> LoadStateFromDisk() const;
    void LoadState() override;
    void SaveStateToYt(const NProto::TMasterTargetState& state) const;
    void SaveStateToDisk(const NProto::TMasterTargetState& state) const;
    void SaveState() const override;

    void TryPoke(TWorkerPool* pool);

public:
    void AddMailRecipient(const TString& email);
    void AddSmsRecipient(const TString& account);
    void AddTelegramRecipient(const TString& account);
    void AddJNSChannelRecipient(const TString& account);
    void AddJugglerEventTag(const TString& tag);

    const TRecipientsWithTransport* GetRecipients() const {
        return Recipients.Get();
    }

    bool UpdatePokeState();

    struct TCheckConditionsFailed: yexception {};
    void CheckConditions(ui32 flags, TTraversalGuard& guard, const IWorkerPoolVariables* pool) const;

    const TMasterTaskStatus& GetTaskStatusByWorkerIdAndLocalTaskN(THostIdSafe hostId, ui32 taskN) const;
    TMasterTaskStatus& GetTaskStatusByWorkerIdAndLocalTaskN(THostIdSafe hostId, ui32 taskN);

    const TVector<TString>& GetPinneds() const noexcept { return Pinneds; }

    TRevision::TValue GetRevision() const noexcept { return Revision; }

    void DumpStateExtra(TPrinter& out) const override;
    void SetTaskState(const TString& worker, const TString& taskName, const TTaskState& state);
    void SetTaskState(const TVector<TString>& taskPath, const TTaskState& state);
    void SetAllTaskStateOnWorker(const TString& worker, const TTaskState& state);
    void SetAllTaskState(const TTaskState& state);

    TTaskIterator TaskIteratorForHost(THostId hostId) {
        return TaskIteratorForLevel(
                Type->GetParameters().GetLevelId(TTargetTypeParameters::HOST_LEVEL_ID),
                Type->GetHostIdSafe(hostId));
    }

    TConstTaskIterator TaskIteratorForHost(THostId hostId) const {
        return TaskIteratorForLevel(
                Type->GetParameters().GetLevelId(TTargetTypeParameters::HOST_LEVEL_ID),
                Type->GetHostIdSafe(hostId));
    }

    TFlatConstTaskIterator TaskFlatIteratorForHost(THostId hostId) const {
        return TFlatConstTaskIterator(this, TMaybe<TTargetTypeParameters::TParamId>(Type->GetHostIdSafe(hostId)));
    }

    TTaskIterator TaskIteratorForHost(const TString& host) {
        return TaskIteratorForHost(Type->GetHostId(host));
    }

    TConstTaskIterator TaskIteratorForHost(const TString& host) const {
        return TaskIteratorForHost(Type->GetHostId(host));
    }

    TTaskIterator TaskIteratorForCluster(TTaskIdSafe taskId) {
        return TaskIteratorForLevel(
                Type->GetParameters().GetLevelId(TTargetTypeParameters::FIRST_PARAM_LEVEL_ID), taskId);
    }

    TConstTaskIterator TaskIteratorForCluster(TTaskIdSafe taskId) const {
        return TaskIteratorForLevel(
                Type->GetParameters().GetLevelId(TTargetTypeParameters::FIRST_PARAM_LEVEL_ID), taskId);
    }

    void CheckState() const override /* override */;

    const TStateCounter& GetStateCounter() { return TargetStats.GetStateCounter(); }
    const TTargetByWorkerStats& GetTargetStats() { return TargetStats; }

    const THolder<TCronEntry>& GetRestartOnSuccessSchedule() const { return RestartOnSuccessSchedule; }
    void SetRestartOnSuccessSchedule(const TString& scheduleStr) { RestartOnSuccessSchedule.Reset(new TCronEntry(scheduleStr)); }
    bool GetRestartOnSuccessEnabled() const { return RestartOnSuccessEnabled; }
    void SetRestartOnSuccessEnabled(bool restartOnSuccessEnabled) { RestartOnSuccessEnabled = restartOnSuccessEnabled; }

    const THolder<TCronEntry>& GetRetryOnFailureSchedule() const { return RetryOnFailureSchedule; }
    void SetRetryOnFailureSchedule(const TString& scheduleStr) { RetryOnFailureSchedule.Reset(new TCronEntry(scheduleStr)); }
    bool GetRetryOnFailureEnabled() const { return RetryOnFailureEnabled; }
    void SetRetryOnFailureEnabled(bool retryOnFailureEnabled) { RetryOnFailureEnabled = retryOnFailureEnabled; }

    TCronState& CronStateForHost(const TString& host);
    bool CronFailedOnHost(const TString& host) const;
    ECronState CronState() const;
    bool CronFailedOnAnyHost() const;

private:
    void SendFailureEmail(NAsyncJobs::TSendMailJob::EType type, const TString& worker, size_t task);
    bool IsCompletelyFinished(bool& success, TTraversalGuard& guard);
    bool IsIdle();

    void InsertCronState(const TString& host, const TCronState& restartStateChangedTime);
    NProto::TMasterTargetState SerializeStateToProtobuf() const;
    const TFsPath GetStateFilePath() const;

private:
    TTargetByWorkerStats TargetStats;

    THolder<TRecipientsWithTransport> Recipients;

    TVector<TString> Pinneds;

    TRevision Revision;

    typedef TMap<TString, TCronState> TCronStateByHost;
    TCronStateByHost CronStateByHost;
    TSpinLock CronStateByHostLock;

    THolder<TCronEntry> RestartOnSuccessSchedule;
    bool RestartOnSuccessEnabled;

    THolder<TCronEntry> RetryOnFailureSchedule;
    bool RetryOnFailureEnabled;

    bool HasDepfailPokes;

private:
    TString TargetDescription;
public:
    void SetTargetDescription(const TString& text){TargetDescription = text;};
    TString GetTargetDescription(){return TargetDescription;};
};
