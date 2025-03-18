#include "master_target.h"

#include "command_alias.h"
#include "log.h"
#include "make_vector.h"
#include "master.h"
#include "master_target_graph.h"
#include "messages.h"
#include "state_file.h"
#include "target_impl.h"
#include "vector_to_string.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/proto/target.pb.h>

#include <library/cpp/deprecated/split/split_iterator.h>

#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/system/fs.h>

template<>
TString ToString<ECronState>(const ECronState& s) {
    switch (s) {
    case ECronState::Failed:    return "cron_failed";
    case ECronState::Succeeded: return "cron_succeeded";
    }
}

TMasterTarget::TMasterTarget(const TString& n, TMasterTargetType* t, const TParserState& parserState)
    : TTargetBase<TMasterGraphTypes>(n, t, parserState)
    , TargetStats(*t)
    , RestartOnSuccessSchedule(nullptr)
    , RestartOnSuccessEnabled(true)
    , RetryOnFailureSchedule(nullptr)
    , RetryOnFailureEnabled(MasterOptions.RetryOnFailureEnabled)
    , HasDepfailPokes(false)
{
    // initially, i.e. without any updates from workers, all
    // tasks are in unknown state
    SetAllTaskState(TS_UNKNOWN);
}

void TMasterTarget::RegisterDependency(TMasterTarget* depend, TTargetParsed::TDepend& dependParsed, const TParserState& state,
        TMasterGraphTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer)
{
    if (dependParsed.Flags & DF_GROUP_DEPEND) {
        for (TVector<TString>::const_iterator i = Type->GetHosts().begin(); i != Type->GetHosts().end(); i++)
            if (!Type->Graph->GetListManager()->ValidateGroupHost(*i))
                ythrow yexception() << "Invalid group dependency " << Name << " -> " << depend->Name << " (non-group host " << *i << ")";
        for (TVector<TString>::const_iterator i = depend->Type->GetHosts().begin(); i != depend->Type->GetHosts().end(); i++)
            if (!Type->Graph->GetListManager()->ValidateGroupHost(*i))
                ythrow yexception() << "Invalid group dependency " << Name << " -> " << depend->Name << " (non-group host " << *i << ")";
    }

    TTargetBase<TMasterGraphTypes>::RegisterDependency(depend, dependParsed, state, precomputedTaskIdsContainer);

    if (Depends.back().IsCrossnode()) {
        HasCrossnodeDepends = true;
    }

    // Retrieving normal and invert edges

    TMasterDepend& normal = Depends.back();
    TMasterDepend& invert = depend->Followers.back();

    // Initializing PrecomputedTaskIds for all edges

    TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<TMasterGraphTypes> > taskIdsMaybe;

    if (normal.GetMode() == DM_PROJECTION) {
        Y_VERIFY(invert.GetMode() == DM_PROJECTION, "Both normal and invert edges must have the same mode");
        TSimpleSharedPtr<IPrecomputedTaskIdsInitializer> initializer = precomputedTaskIdsContainer->Container.GetOrCreate(
                TDependSourceTypeTargetTypeAndMappings<TMasterTargetType>(*normal.GetJoinParamMappings(), normal.GetSource()->Type, normal.GetTarget()->Type));
        initializer->Initialize();
        taskIdsMaybe.Reset(TPrecomputedTaskIdsMaybe<TMasterGraphTypes>::Defined(initializer));
    } else if (normal.GetMode() == DM_GROUPDEPEND) {
        Y_VERIFY(invert.GetMode() == DM_GROUPDEPEND, "Both normal and invert edges must have the same mode");
        TSimpleSharedPtr<TMasterPrecomputedTaskIdsInitializerGroup> initializer = precomputedTaskIdsContainer->ContainerGroup.GetOrCreate(
                TMasterDependSourceTypeTargetType(normal.GetSource()->Type, normal.GetTarget()->Type));
        initializer->Initialize();
        taskIdsMaybe.Reset(TPrecomputedTaskIdsMaybe<TMasterGraphTypes>::Defined(initializer));
    } else {
        Y_FAIL("Bad depend mode");
    }

    normal.SetPrecomputedTaskIdsMaybe(taskIdsMaybe);
    invert.SetPrecomputedTaskIdsMaybe(taskIdsMaybe);
}

void TMasterTarget::AddOption(const TString &key, const TString &value) {
    if (key == "mailto") {
        const auto& mails = SplitRecipients(value);

        for (const auto& mail : mails)
            AddMailRecipient(mail);

        Type->Graph->SetHasObservers();
    } else if (key == "smsto") {
        const auto& accounts = SplitRecipients(value);

        for (const auto& account: accounts)
            AddSmsRecipient(account);

        Type->Graph->SetHasObservers();
    } else if (key == "telegramto") {
        const auto& accounts = SplitRecipients(value);

        for (const auto& account: accounts)
            AddTelegramRecipient(account);

        Type->Graph->SetHasObservers();
    } else if (key == "jns_channel") {
        const auto& accounts = SplitRecipients(value);

        for (const auto& account: accounts) {
            AddJNSChannelRecipient(account);
        }

        Type->Graph->SetHasObservers();
    } else if (key == "juggler") {
        const auto& tags = SplitRecipients(value);

        for (const TString& tag : tags) {
            AddJugglerEventTag(tag);
        }

        Type->Graph->SetHasObservers();
    } else if (key == "graphtag") {
        Y_VERIFY(Followers.empty(), "tag should be defined for final target");

        TAtomicSharedPtr<TTargetSubgraph> tempSubgraph = new TTargetSubgraph();
        NGatherSubgraph::TMask mask(&Type->GetParameters());

        NGatherSubgraph::GatherSubgraph<TMasterGraphTypes, TParamsByTypeOrdinary, TMasterGraph::TMasterTargetEdgeInclusivePredicate>
            (*this, mask, NGatherSubgraph::M_COMMAND_RECURSIVE_UP, tempSubgraph.Get());

        Type->Graph->AddTaggedSubgraph(value, tempSubgraph);
    } else if (key == "pinned") {
        static const char * const AllowedPinnedsTable[] = {
            "run-path",
            "retry-run-path",
            "invalidate-path",
            "invalidate-following",
            "reset-path",
            "reset-following",
            "cancel-path",
            "cancel-following",
        };

        static const TStaticStringSet AllowedPinneds(AllowedPinnedsTable);

        const TSplitDelimiters delims(";,");
        const TDelimitersStrictSplit split(value.data(), value.size(), delims);
        TDelimitersStrictSplit::TIterator it = split.Iterator();

        Pinneds.clear();

        while (!it.Eof()) {
            const TString& token = it.NextString();

            if (AllowedPinneds(token))
                Pinneds.push_back(token);
        }
    } else if (key == "restart_on_success") {
        SetRestartOnSuccessSchedule(value);
    } else if (key == "retry_on_failure") {
        SetRetryOnFailureSchedule(value);
    } else if (key == "enable_retry_on_failure") {
        SetRetryOnFailureEnabled(IsTrueValue(value));
    } else if (key == "description") {
        SetTargetDescription(value);
    } else if (key == "res") {
    } else {
        ERRORLOG("Unknown target option " << key);
    }
}

void TMasterTarget::AddMailRecipient(const TString& email) {
    if (Recipients.Get() == nullptr)
        Recipients.Reset(new TRecipientsWithTransport());

    Recipients->AddMailRecipient(email);
}

void TMasterTarget::AddSmsRecipient(const TString& account) {
    if (Recipients.Get() == nullptr)
        Recipients.Reset(new TRecipientsWithTransport());

    Recipients->AddSmsRecipient(account);
}

void TMasterTarget::AddJNSChannelRecipient(const TString& account) {
    if (Recipients.Get() == nullptr) {
        Recipients.Reset(new TRecipientsWithTransport());
    }

    Recipients->AddJNSChannelRecipient(account);
}

void TMasterTarget::AddTelegramRecipient(const TString& account) {
    if (Recipients.Get() == nullptr)
        Recipients.Reset(new TRecipientsWithTransport());

    Recipients->AddTelegramRecipient(account);
}

void TMasterTarget::AddJugglerEventTag(const TString& tag) {
    if (Recipients.Get() == nullptr)
        Recipients.Reset(new TRecipientsWithTransport());

    Recipients->AddJugglerEventTag(tag);
}

bool TMasterTarget::IsCompletelyFinished(bool& success, TTraversalGuard& guard) {
    if (!GetStateCounter().IsCompletely(TS_SUCCESS))
        success = false;

    if (GetStateCounter().HasState(TS_RUNNING) || GetStateCounter().HasState(TS_CANCELING) ||
            GetStateCounter().HasState(TS_PENDING) || GetStateCounter().HasState(TS_READY))
    {
        return false;
    }

    for (TDependsList::iterator depend = Depends.begin(); depend != Depends.end(); ++depend)
        if (guard.TryVisit(depend->GetTarget()) && !depend->GetTarget()->IsCompletelyFinished(success, guard))
            return false;

    return true;
}

bool TMasterTarget::IsIdle() {
    return GetStateCounter().GetCount(TS_SUCCESS) + GetStateCounter().GetCount(TS_IDLE) == GetStateCounter().GetTotalCount();
}

void TMasterTarget::TryPoke(TWorkerPool* pool) {
    // We have nothing to poke unless there are pending targets
    if (!GetStateCounter().HasState(TS_PENDING))
        return;

    for (THostId hostId = 0; hostId < Type->GetHostCount(); ++hostId) {
        const TString& workerName = Type->GetHostById(hostId);

        TMultiPokeMessage pokeReadyMessage(Name, TPokeMessage::PF_READY);

        int nTask = 0;
        bool pokeReadyAllTasks = true;
        for (TConstTaskIterator task = TaskIteratorForHost(hostId); task.Next(); ++nTask) {
            if (task->Status.GetState() == TS_PENDING) {
                if (task->PokeReady)
                    pokeReadyMessage.AddTasks(nTask);
                else
                    pokeReadyAllTasks = false;
            }
        }

        if (pokeReadyMessage.TasksSize() > 0 && pokeReadyAllTasks)
            pool->EnqueueMessageToWorker(workerName, TPokeMessage(Name, TPokeMessage::PF_READY));
        else if (pokeReadyMessage.TasksSize() > 0)
            pool->EnqueueMessageToWorker(workerName, pokeReadyMessage);
    }
}

bool TMasterTarget::UpdatePokeState() {
    if (!HasCrossnodeDepends) {
        return false;
    }

    bool has_pending_tasks = false;

    // Pre-set ready state
    for (TMasterTarget::TTaskIterator task = TaskIterator(); task.Next(); ) {
        if (task->Status.GetState() == TS_PENDING) {
            has_pending_tasks = true;
            task->PokeReady = true;
        } else {
            task->PokeReady = false;
        }
    }

    // Don't have anything runnable
    if (!has_pending_tasks)
        return false;

    for (TDependsList::iterator depend = Depends.begin(); depend != Depends.end(); ++depend) {
        IPrecomputedTaskIdsInitializer* precomputed = depend->GetPrecomputedTaskIdsMaybe()->Get();
        // precomputed->Initialize();

        for (TDependEdgesEnumerator en(precomputed->GetIds()); en.Next(); ) {
            bool allDependsSucceeded = true;

            for (TPrecomputedTaskIdsForOneSide::TIterator
                    depTaskIndex = en.GetDepTaskIds().Begin();
                    depTaskIndex != en.GetDepTaskIds().End();
                    ++depTaskIndex)
            {
                const TSpecificTaskStatus *depTask = &depend->GetTarget()->Tasks.At(*depTaskIndex);

                if (depTask->Status.GetState() != TS_SUCCESS && depTask->Status.GetState() != TS_SKIPPED) {
                    allDependsSucceeded = false;
                    break;
                }
            }

            if (!allDependsSucceeded) {
                for (TPrecomputedTaskIdsForOneSide::TIterator
                        myTaskIndex = en.GetMyTaskIds().Begin();
                        myTaskIndex != en.GetMyTaskIds().End();
                        ++myTaskIndex)
                {
                    TSpecificTaskStatus *myTask = &depend->GetSource()->Tasks.At(*myTaskIndex);
                    myTask->PokeReady = false;
                }
            }
        }
    }

    for (TTaskIterator task = TaskIterator(); task.Next(); ) {
        if (task->PokeReady) {
            return true;
        }
    }

    return false;
}

struct TChecker {
    const TCondition& Condition;
    const IWorkerPoolVariables& Pool;
    const TString& Target;
    const TString& Depend;

    bool HasTrue;
    bool HasFalse;

    TChecker(const TCondition& condition, const IWorkerPoolVariables& pool, const TString& target, const TString& depend)
        : Condition(condition)
        , Pool(pool)
        , Target(target)
        , Depend(depend)
        , HasTrue(false)
        , HasFalse(false)
    {
    }

    void operator()(const TString& worker) {
        const bool ret = Pool.GetVariablesForWorker(worker).CheckCondition(Condition);

        if (ret)
            HasTrue = true;
        else
            HasFalse = true;

        if (HasTrue && HasFalse)
            throw TMasterTarget::TCheckConditionsFailed() << Target << ", dependence " << Depend << ": condition " <<
            Condition.GetOriginal() << " should be " << (ret ? "false" : "true") << " on worker " << worker;
    }
};

void CheckConditionsForAllDeps(const IWorkerPoolVariables* pool, const TMasterTarget::TDependsList& list, ui32 flags, TMasterTarget::TTraversalGuard& guard) {
    for (TMasterTarget::TDependsList::const_iterator dep = list.begin(); dep != list.end(); ++dep) {
        if (dep->IsCrossnode() && !dep->GetCondition().IsEmpty()) {
            TChecker check(dep->GetCondition(), *pool, dep->GetRealSource()->Name, dep->GetRealTarget()->Name);

            for (TIdForString::TIterator worker = dep->GetRealSource()->Type->GetHostList().Iterator(); worker.Next();)
                if (pool->CheckIfWorkerIsAvailable(worker.GetName()))
                    check(worker.GetName());

            for (TIdForString::TIterator worker = dep->GetRealTarget()->Type->GetHostList().Iterator(); worker.Next();)
                if (pool->CheckIfWorkerIsAvailable(worker.GetName()))
                    check(worker.GetName());

            dep->GetTarget()->CheckConditions(flags, guard, pool);
        }
    }
}

void TMasterTarget::CheckConditions(ui32 flags, TTraversalGuard& guard, const IWorkerPoolVariables* pool) const {
    if (!guard.TryVisit(this))
        return;

    if (flags & TCommandMessage::CF_RECURSIVE_UP)
        CheckConditionsForAllDeps(pool, Depends, flags, guard);

    if (flags & TCommandMessage::CF_RECURSIVE_DOWN)
        CheckConditionsForAllDeps(pool, Followers, flags, guard);
}

const TMasterTaskStatus& TMasterTarget::GetTaskStatusByWorkerIdAndLocalTaskN(TMasterTarget::THostIdSafe hostId, ui32 taskN) const {
    return const_cast<TMasterTarget*>(this)->GetTaskStatusByWorkerIdAndLocalTaskN(hostId, taskN);
}

TMasterTaskStatus& TMasterTarget::GetTaskStatusByWorkerIdAndLocalTaskN(TMasterTarget::THostIdSafe hostId, ui32 taskN) {
    return Tasks.At(Type->GetTaskIdByWorkerIdAndLocalTaskN(hostId, taskN));
}

void TMasterTarget::DumpStateExtra(TPrinter& out) const {
    out.Println("State for hosts:");
    TPrinter l1 = out.Next();
    for (TMasterTarget::THostId hostId = 0; hostId < Type->GetHostCount(); ++hostId) {
        const TString& workerName = Type->GetHostById(hostId);
        l1.Println(workerName + ":");
        for (TConstTaskIterator task = TaskIteratorForHost(hostId); task.Next(); ) {
            TPrinter l2 = l1.Next();
            l2.Println(ToString(task.GetPath())
                    + " " + (task->PokeReady ? "poke=yes" : "poke=no")
                    + " " + (task->PokeDepfail ? "poke_depfail=yes" : "poke_depfail=no")
                    + " " + ToString(task->Status.GetState())
                    );
        }
    }
}

void TMasterTarget::CheckState() const {
    TTargetBase<TMasterGraphTypes>::CheckState();
}

void TMasterTarget::SetTaskState(const TString& worker, const TString& taskName, const TTaskState& state) {
    SetTaskState(MakeVector<TString>(worker, taskName), state);
}

void TMasterTarget::SetTaskState(const TVector<TString>& taskPath, const TTaskState& state) {
    TTaskId taskId = Type->GetParameters().GetNForPath(taskPath);
    Tasks.At(taskId).Status.SetState(state);
}

void TMasterTarget::SetAllTaskStateOnWorker(const TString& worker, const TTaskState& state) {
    for (TMasterTarget::TTaskIterator task = TaskIteratorForHost(worker); task.Next(); ) {
        task->Status.SetState(state);
    }
}

void TMasterTarget::SetAllTaskState(const TTaskState& state) {
    for (TMasterTarget::TTaskIterator task = TaskIterator(); task.Next(); ) {
        task->Status.SetState(state);
    }
}

bool TMasterTarget::IsSame(const TMasterTarget* right) const {
    return Name == right->Name;
}

void TMasterTarget::InsertCronState(const TString& host, const TCronState& rsct) {
    if (!Type->HasHostname(host)) {
        LOG("Now target '" << GetName() << "' has no host named '" << host << "' - ignoring restart state for this host");
    } else {
        CronStateByHost.insert(std::make_pair(host, rsct));
    }
}

void TMasterTarget::SwapState(TMasterTarget* right) {
    SetRestartOnSuccessEnabled(right->GetRestartOnSuccessEnabled());
    SetRetryOnFailureEnabled(right->GetRetryOnFailureEnabled());

    {
        TGuard<TSpinLock> guard(CronStateByHostLock);

        for (TCronStateByHost::const_iterator i = right->CronStateByHost.begin(); i != right->CronStateByHost.end(); i++) {
            InsertCronState(i->first, i->second);
        }
    }
}

NProto::TMasterTargetState TMasterTarget::SerializeStateToProtobuf() const {
    NProto::TMasterTargetState state;
    try {
        state.SetRestartOnSuccessEnabled(GetRestartOnSuccessEnabled());
        state.SetRetryOnFailureEnabled(GetRetryOnFailureEnabled());

        {
            TGuard<TSpinLock> guard(CronStateByHostLock);
            state.SetUpdateTimestamp(MilliSeconds());

            for (TCronStateByHost::const_iterator i = CronStateByHost.begin(); i != CronStateByHost.end(); i++) {
                NProto::TTargetWorkerRestartState *workerState = state.AddStateByWorker();
                workerState->SetHost(i->first);
                workerState->SetRestartState(i->second.GetState());
            }
        }
    } catch (const yexception& e) {
        ythrow yexception() << "cannot save state for target " << GetName() << ": " << e.what();
    }
    return state;
}

const TFsPath TMasterTarget::GetStateFilePath() const {
    return StateFilePath(MasterOptions.VarDirPath, GetName());
}

TMaybe<NProto::TMasterTargetState> TMasterTarget::LoadStateFromYt() const {
    TMaybe<TYtStorage>& storage = MasterOptions.YtStorage;
    if (!storage) {
        return {};
    }
    return storage->LoadAndParseStateFromYt<NProto::TMasterTargetState>(GetName());
}

TMaybe<NProto::TMasterTargetState> TMasterTarget::LoadStateFromDisk() const {
    return ::LoadAndParseStateFromDisk<NProto::TMasterTargetState>(GetStateFilePath());
}

void TMasterTarget::LoadState() {
    if (!GetRestartOnSuccessSchedule().Get() && !GetRetryOnFailureSchedule().Get())
        return;

    TVector<TMaybe<NProto::TMasterTargetState>> candidates;
    candidates.emplace_back(LoadStateFromYt());
    candidates.emplace_back(LoadStateFromDisk());

    TMaybe<NProto::TMasterTargetState> newest = ::SelectByUpdateTimestamp(candidates);
    if (newest.Empty()) {
        ERRORLOG("Failed to load state for target " << GetName() << " from any source");
        return;
    }

    try {
        SetRestartOnSuccessEnabled(newest->GetRestartOnSuccessEnabled());
        SetRetryOnFailureEnabled(newest->GetRetryOnFailureEnabled());

        {
            TGuard<TSpinLock> guard(CronStateByHostLock);

            typedef google::protobuf::RepeatedPtrField<NProto::TTargetWorkerRestartState> TTargetWorkerRestartState;
            for (TTargetWorkerRestartState::const_iterator i = newest->GetStateByWorker().begin(); i != newest->GetStateByWorker().end(); i++) {
                TCronState rsct = TCronState(i->GetRestartState(), TInstant::Now()); // don't want to save real changed time to protobuf - it's ok
                        // if changed time would be set to Now() on master restart
                InsertCronState(i->GetHost(), rsct);
            }
        }
    } catch (const yexception& e) {
        ythrow yexception() << "cannot load state for target " << Name << ": " << e.what();
    }
}

void TMasterTarget::SaveStateToYt(const NProto::TMasterTargetState& protoState) const {
    TMaybe<TYtStorage>& storage = MasterOptions.YtStorage;
    if (storage) {
        storage->SaveProtoStateToYt(GetName(), protoState);
    }
}

void TMasterTarget::SaveStateToDisk(const NProto::TMasterTargetState& protoState) const {
    ::SaveProtoStateToDisk(GetStateFilePath(), protoState);
}

void TMasterTarget::SaveState() const {
    if ((!GetRestartOnSuccessSchedule().Get() && !GetRetryOnFailureSchedule().Get()) || Type->Graph->ForTest)
        return;

    NProto::TMasterTargetState protoState = SerializeStateToProtobuf();
    SaveStateToDisk(protoState);
    SaveStateToYt(protoState);
}

TCronState& TMasterTarget::CronStateForHost(const TString& host) {
    TGuard<TSpinLock> guard(CronStateByHostLock);
    return CronStateByHost[host];
}

bool TMasterTarget::CronFailedOnHost(const TString& host) const {
    TGuard<TSpinLock> guard(CronStateByHostLock);

    TCronStateByHost::const_iterator iter = CronStateByHost.find(host);
    if (iter == CronStateByHost.end())
        return false;
    else
        return iter->second.IsFailed();
}

bool TMasterTarget::CronFailedOnAnyHost() const {
    TGuard<TSpinLock> guard(CronStateByHostLock);

    bool result = false;

    for (TCronStateByHost::const_iterator i = CronStateByHost.begin(); i != CronStateByHost.end(); ++i) {
        if (i->second.IsFailed()) {
            result = true;
            break;
        }
    }

    return result;
}

ECronState TMasterTarget::CronState() const {
    if (CronFailedOnAnyHost()) {
        return ECronState::Failed;
    }
    return ECronState::Succeeded;
}
