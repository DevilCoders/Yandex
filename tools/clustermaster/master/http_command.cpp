#include "command_alias.h"
#include "http.h"
#include "http_common.h"
#include "log.h"
#include "master.h"
#include "master_target_graph.h"
#include "messages.h"
#include "worker_pool.h"

void TMasterHttpRequest::CheckConditionIfNeeded(IOutputStream& out, ui32 flags, TMasterGraph::TTargetsList::const_iterator target,
        const IWorkerPoolVariables* pool)
{
    if ((flags & TCommandMessage::CF_RUN) && (flags & (TCommandMessage::CF_RECURSIVE_UP | TCommandMessage::CF_RECURSIVE_DOWN))) {
        TMasterTarget::TTraversalGuard guard;
        try {
            (*target)->CheckConditions(flags, guard, pool);
        } catch (const TMasterTarget::TCheckConditionsFailed& e) {
            ServeSimpleStatus(out, HTTP_BAD_REQUEST, e.what());
            return;
        }
    }
}

void TMasterHttpRequest::ServeCommand(IOutputStream& out, const TString& cmd) {
    TLockableHandle<TMasterGraph>::TWriteLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TWriteLock pool(GetPool());

    typedef TVector<TWorker*> WorkersList;
    typedef TVector<TMasterGraph::TTargetsList::const_iterator> TargetsList;
    typedef TVector<int> TasksList;
    typedef TSet<TString> WorkerHostsList;

    WorkersList workers;
    TargetsList targets;
    TasksList ntasks;

    WorkerHostsList worker_hosts;

    TTaskState state = TS_UNDEFINED;

    bool all_workers = false;

    if (cmd == "reload") {
        LOG1(http, RequesterName << ": Command reload, raising flag");
        MasterEnv.NeedReloadScript = true;
    } else if (cmd == "exit") {
        LOG1(http, RequesterName << ": Command exit, raising flag");
        MasterEnv.NeedExit = true;
    } else {
        ui32 flags = 0;

        try {
            flags = NCA::GetFlags(cmd);
        } catch (const NCA::TNoAlias& e) {
            ServeSimpleStatus(out, HTTP_BAD_REQUEST, TString("Bad command \"") + EncodeXMLString(cmd.data()) + '\"');
            return;
        }

        for (TQueryMap::iterator i = Query.begin(); i != Query.end(); ++i) {
            if (i->first == "worker") {
                TVector<TString> args;
                Split(i->second, ",", args);

                for (TVector<TString>::const_iterator i = args.begin(); i != args.end(); ++i) {
                    TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().find(*i);
                    if (worker == pool->GetWorkers().end()) {
                        ServeSimpleStatus(out, HTTP_BAD_REQUEST, TString("Wrong worker host ") + EncodeXMLString(i->data()));
                        return;
                    }

                    workers.push_back(*worker);
                    worker_hosts.insert((*worker)->GetHost());
                }
            } else if (i->first == "target") {
                TVector<TString> args;
                Split(i->second, ",", args);

                for (TVector<TString>::const_iterator i = args.begin(); i != args.end(); ++i) {
                    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(*i);
                    if (target == graph->GetTargets().end()) {
                        ServeSimpleStatus(out, HTTP_BAD_REQUEST, TString("Wrong target name ") + EncodeXMLString(i->data()));
                        return;
                    }

                    targets.push_back(target);
                }
            } else if (i->first == "task") {
                try {
                    TVector<TString> args;
                    Split(i->second, ",", args);

                    for (TVector<TString>::const_iterator i = args.begin(); i != args.end(); ++i)
                        ntasks.push_back(FromString<unsigned>(*i));
                } catch (const yexception& /*e*/) {
                    ServeSimpleStatus(out, HTTP_BAD_REQUEST, TString("Wrong task number(s) ") + EncodeXMLString(i->second.data()));
                    return;
                }
            } else if (i->first == "state") {
                try {
                    state = FromString<TTaskState>(i->second);
                } catch (const yexception& /*e*/) {
                    ServeSimpleStatus(out, HTTP_BAD_REQUEST, TString("Wrong state ") + EncodeXMLString(i->second.data()));
                    return;
                }
            } else if (i->first == "all_workers"){
                all_workers = true;
            }
        }

        // Target should be specified
        if (targets.empty()) {
            ServeSimpleStatus(out, HTTP_BAD_REQUEST, "Target specification required");
            return;
        }

        if (all_workers) {
            if (!(flags & (TCommandMessage::CF_RECURSIVE_UP | TCommandMessage::CF_RECURSIVE_DOWN))) {
                ServeSimpleStatus(out, HTTP_BAD_REQUEST, "Requesting non-recursive command to be sent on all workers");
                return;
            }

            for (TargetsList::const_iterator target = targets.begin(); target != targets.end(); ++target) {
                CheckConditionIfNeeded(out, flags, *target, pool.Get());

                TCommandMessage2 msg((**target)->Name, flags);

                TMaybe<TVector<int> > explicitTaskList;

                if (workers.empty() && ntasks.empty()) {
                    explicitTaskList = TMaybe<TVector<int> >(); // empty task list means all tasks on all workers
                } else { // any other case leads to explicit list of task ids
                    TVector<TWorker*> workers2;
                    if (workers.empty()) { // empty workers list means every worker
                        for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
                            workers2.push_back(*worker);
                        }
                    } else {
                        workers2 = workers;
                    }

                    explicitTaskList = TVector<int>();

                    for (TVector<TWorker*>::const_iterator worker = workers2.begin(); worker != workers2.end(); ++worker) {
                        const TString& host = (*worker)->GetHost();

                        if (!(**target)->Type->HasHostname(host)) {
                            TString error;
                            TStringOutput errorOut(error);
                            errorOut << "Target " << (**target)->GetName() << " type has no " << host << " host";
                            ServeSimpleStatus(out, HTTP_BAD_REQUEST, error);
                            return;
                        }

                        if (ntasks.empty()) { // empty tasks list means every task for given worker
                            for (TMasterTarget::TConstTaskIterator i = (**target)->TaskIteratorForHost(host); i.Next(); ) {
                                explicitTaskList->push_back(i.GetN().GetN());
                            }
                        } else {
                            size_t taskCountForHost = (**target)->Type->GetTaskCountByWorker(host);
                            for (TasksList::const_iterator task = ntasks.begin(); task != ntasks.end(); ++task) {
                                if (size_t(*task) > taskCountForHost) {
                                    TString error;
                                    TStringOutput errorOut(error);
                                    errorOut << "Task id " << *task << " is greater than task count for worker " << host << " (" << taskCountForHost << ")";
                                    ServeSimpleStatus(out, HTTP_BAD_REQUEST, error);
                                    return;
                                }
                                TMasterTargetType::TLocalspaceShifts::const_iterator shift = (**target)->Type->GetLocalspaceShifts().find(host);
                                Y_VERIFY(shift != (**target)->Type->GetLocalspaceShifts().end());
                                explicitTaskList->push_back(*task + shift->second);
                            }
                        }
                    }
                }

                TMaybe<TVector<int> > explicitTaskListFilteredByState;

                if (state == TS_UNDEFINED) {
                    explicitTaskListFilteredByState = explicitTaskList;
                } else {
                    if (explicitTaskList.Defined()) {
                        explicitTaskListFilteredByState = TVector<int>();
                        for (TVector<int>::const_iterator i = explicitTaskList->begin(); i != explicitTaskList->end(); ++i) {
                            if ((**target)->GetTasks().At(*i).Status.GetState() == state) {
                                explicitTaskListFilteredByState->push_back(*i);
                            }
                        }
                    } else {
                        bool allTaskAreOk = true;
                        for (TMasterTarget::TConstTaskIterator task = (**target)->TaskIterator(); task.Next(); ) {
                            if (task->Status.GetState() != state) {
                                allTaskAreOk = false;
                                break;
                            }
                        }
                        if (allTaskAreOk) {
                            explicitTaskListFilteredByState = TMaybe<TVector<int> >(); // means all tasks for target
                        } else {
                            explicitTaskListFilteredByState = TVector<int>();
                            for (TMasterTarget::TConstTaskIterator task = (**target)->TaskIterator(); task.Next(); ) {
                                if (task->Status.GetState() == state)
                                    explicitTaskListFilteredByState->push_back(task.GetN().GetN());
                            }
                        }
                    }
                }

                if (explicitTaskListFilteredByState.Defined() && explicitTaskListFilteredByState->empty()) {
                    TString error;
                    TStringOutput errorOut(error);
                    errorOut << "After filtering tasks by state no tasks left";
                    ServeSimpleStatus(out, HTTP_BAD_REQUEST, error);
                    return;
                }

                if (explicitTaskListFilteredByState.Defined()) {
                    for (TVector<int>::const_iterator i = explicitTaskListFilteredByState->begin(); i != explicitTaskListFilteredByState->end(); ++i)
                        msg.AddTask(*i);
                }

                pool->EnqueueMessageAll(msg); // We send message to all workers in any case. Workers have full graph on their
                        // side (but without statuses) - this allows to calculate what need to be done on workers. This is needed
                        // to 1) have smaller network messages (we need to send tasks lists - maybe compressed somehow - if
                        // calculation is done on master side 2) decrease master's load (graph traverse is complicated task)

                LOG1(http, RequesterName << ": Command2 (" << msg.FormatText() << "), enqueuing for all");
            }
        } else {
            // If no tasks specified, use all tasks (-1 means 'all tasks')
            if (ntasks.empty()) {
                ntasks.push_back(-1);
            }

            for (TargetsList::const_iterator target = targets.begin(); target != targets.end(); ++target) {
                CheckConditionIfNeeded(out, flags, *target, pool.Get());

                for (TasksList::const_iterator task = ntasks.begin(); task != ntasks.end(); ++task) {
                    TCommandMessage msg((**target)->Name, *task, flags, state);

                    if (workers.empty()) { // all workers
                        pool->EnqueueMessageAll(msg);
                        LOG1(http, RequesterName << ": Command (" << msg.FormatText() << "), enqueuing for all");
                    } else { // one worker (or workers list which is rare but possible case) is given
                        for (WorkersList::const_iterator worker = workers.begin(); worker != workers.end(); ++worker) {
                            pool->EnqueueMessageToWorker((*worker)->GetHost(), msg);
                            LOG1(http, RequesterName << ": Command (" << msg.FormatText() << "), enqueuing for " << (*worker)->GetHost());
                        }
                    }
                }
            }
        }
    }

    ServeSimpleStatus(out, HTTP_NO_CONTENT);
}

