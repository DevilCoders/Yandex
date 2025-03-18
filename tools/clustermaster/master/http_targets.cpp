#include "http.h"
#include "http_common.h"
#include "master.h"
#include "master_target_graph.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/gather_subgraph.h>
#include <tools/clustermaster/common/precomputed_task_ids.h>
#include <tools/clustermaster/common/util.h>

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/generic/strbuf.h>
#include <util/string/join.h>

static void PrintDependsAsString(IOutputStream& out, TMasterTarget::TDependsList::const_iterator begin, TMasterTarget::TDependsList::const_iterator end, const TString& delim) {
    for (TMasterTarget::TDependsList::const_iterator i = begin; i != end; ++i) {
        if (i != begin) {
            out << delim;
        }
        out << i->GetTarget()->Name;
    }
}

static void FormatDependsList(IOutputStream& out, TMasterTarget::TDependsList::const_iterator begin, TMasterTarget::TDependsList::const_iterator end) {
    // CSS magic;
    // what we do here is trimming depends list down to a "first_depend, ..."
    // but we make it an such a way that one can still search for trimmed
    // depends with e.g. Ctrl+F
    // For that, we create invisible <div>'s with all depends in a same
    // place as "..."

    if (begin == end)
        out << "&mdash;";

    int len = 0;
    int normaldeps = 0;
    TString divs;
    TString deps;
    for (TMasterTarget::TDependsList::const_iterator i = begin; i != end; ++i) {
        len += i->GetTarget()->Name.length() + (i == begin ? 0 : 2);
        if (len < 30) {
            if (i != begin)
                out << ", ";

            if (i->IsCrossnode())
                out << "<b>" << i->GetTarget()->Name << "</b>";
            else
                out << i->GetTarget()->Name;

            normaldeps++;
        } else {
            if (deps.empty()) {
                if (normaldeps)
                    out << ", ";
                deps += i->GetTarget()->Name;
            } else {
                deps += TString(", ") + i->GetTarget()->Name;
            }

            divs += TString("<div class=\"hidden\">") + i->GetTarget()->Name + "</div>";
        }
    }
    if (!deps.empty())
        out << "<span title=\"" << deps << "\">" << divs << "...</span>";
}

template<typename TTargetsList>
    void TMasterHttpRequest::FormatTargetsList(IOutputStream& out, const TTargetsList& targets, const TWorker* worker, TStateFilter& stateFilter, size_t& number) {
    for (const auto& target : targets) {
        ui32 taskCount = 0;

        if (worker) {
            // Skip targets not running on this worker
            if (!target->Type->HasHostname(worker->GetHost())) {
                continue;
            }

            taskCount = target->Type->GetTaskCountByWorker(worker->GetHost());
        }

        out << "<tr name=\"@@\" number=\"" << number << "\" type=\"" << target->Type->GetName()
            << "\" target=\"" << target->Name << "\" id=\"target_" << target->Name << '"';
        if (stateFilter.IsEnabled())
            out << " style=\"display:none\"";
        out << ">";

        out << "<td class=\"number\">" << number << "</td>";
        out << "<td class=\"type\">" << target->Type->GetName() << "</td>";

        out << "<td class=\"target\">";
        // Name
        if (worker) {
            out << "<a href=\"" << UrlRoot << "worker/" << worker->GetHost() << "/" << target->Name << "\"";
            out << " name=\"AM\" cmdparams=\"worker=" << worker->GetHost() << "&target=" << target->Name << "\">" << target->Name << "</a>";
        } else {
            out << "<a href=\"" << UrlRoot << "target/" << target->Name << "\" name=\"AM\" cmdparams=\"target=" << target->Name << "\">" << target->Name << "</a>";
        }
        out << "</td>";

        // Action
        out << "<td>";
        if (worker && taskCount == 1) {
            out << "<a href=\"" << UrlRoot << "proxy/" << worker->GetHost() << "/logs/" << target->Name << "/0\">Log</a>";
            out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << worker->GetHost() << "/fullogs/" << target->Name << "/0\">Full log</a>";
        } else if (target->Type->GetParameters().GetCount() == 1) {
            out << "<a href=\"" << UrlRoot << "proxy/" << target->Type->GetSingleHost() << "/logs/" << target->Name << "/0\">Log</a>";
            out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << target->Type->GetSingleHost() << "/fullogs/" << target->Name << "/0\">Full log</a>";
        } else {
            out << "&nbsp;";
        }
        // (target has attached mail recipients)
        if (target->GetRecipients() != nullptr) {
            const TRecipients* mailRecipients = target->GetRecipients()->GetMailRecipients();
            if (mailRecipients != nullptr) {
                out << " (<a href=\"mailto:" << JoinSeq(",", *mailRecipients) << "\">@</a>)";
            }
        }

        // target should be restarted on success
        if (target->GetRestartOnSuccessSchedule().Get())
            out << " <span title=\"Restart whole path on success: `" << *target->GetRestartOnSuccessSchedule().Get() << "'\"><a href='#' class='restart-" << (target->GetRestartOnSuccessEnabled() ? "enabled" : "disabled") << "' onclick='toggleRestartOnSuccess(this, \"" << target->Name << "\"); return false'>(*)</a></span>";

        // target should be retried on failure
        if (target->GetRetryOnFailureSchedule().Get())
            out << " <span title=\"Retry run on failure: `" << *target->GetRetryOnFailureSchedule().Get() << "'\"><a href='#' class='restart-" << (target->GetRetryOnFailureEnabled() ? "enabled" : "disabled") << "' onclick='toggleRetryOnFailure(this, \"" << target->Name << "\"); return false'>(^)</a></span>";

        out << "</td>";

        // Communism
        if (worker && taskCount == 1) {
            out << "<td class=\"smarthint\" name=\"SH\" hinturl=\"" << UrlRoot << "proxy/" << worker->GetHost() << "/res/" << target->Name << "/0\">&#9733;</td>";
        } else if (target->Type->GetParameters().GetCount() == 1) {
            out << "<td class=\"smarthint\" name=\"SH\" hinturl=\"" << UrlRoot << "proxy/" << target->Type->GetSingleHost() << "/res/" << target->Name << "/0\">&#9733;</td>";
        } else {
            out << "<td>&nbsp;</td>";
        }

        // State
        out << "<td name=\"@X\"></td>";

        // Depends
        out << "<td class=\"dependers\" targets=\"";
        PrintDependsAsString(out, target->Depends.begin(), target->Depends.end(), ",");
        out << "\">";
        FormatDependsList(out, target->Depends.begin(), target->Depends.end());
        out << "</td>";

        // Followers
        out << "<td class=\"followers\" targets=\"";
        PrintDependsAsString(out, target->Followers.begin(), target->Followers.end(), ",");
        out << "\">";
        FormatDependsList(out, target->Followers.begin(), target->Followers.end());
        out << "</td>";

        if (!worker) {
            // Workers
            out << "<td style=\"text-align:center\" name=\"@W\"></td>";
        }

        // Tasks
        out << "<td style=\"text-align:center\" name=\"@t\"></td>";

        // Info
        if ((worker && taskCount == 1) || target->Type->GetParameters().GetCount() == 1) {
            out << "<td class=\"smarthint\" name=\"IH\" hinturl=\"" << UrlRoot << "info" << UrlString << '/' << target->Name << "\">&#10033;</td>";
        } else {
            out << "<td>&nbsp;</td>";
        }

        // Times
        out << "<td name=\"@s\"></td>";
        out << "<td name=\"@f\"></td>";
        out << "<td name=\"@S\"></td>";
        out << "<td name=\"@F\"></td>";

        // Durations
        if (worker) {
            out << "<td name=\"@dA\"></td>";
            out << "<td name=\"@dt\"></td>";
        } else {
            out << "<td name=\"@dm\"></td>";
            out << "<td name=\"@da\"></td>";
            out << "<td name=\"@dM\"></td>";
        }

        // Resource usage statistics
        out << "<td name=\"@cM\"></td>";
        out << "<td name=\"@ca\"></td>";
        out << "<td name=\"@mM\"></td>";
        out << "<td name=\"@ma\"></td>";

        out << "</tr>";
        ++number;
    }
}

void EnableStateFilterByQuery(TStateFilter* stateFilter, THashMap<TString, TString>& query) {
    if (query.find("state") != query.end())
        stateFilter->Enable(query["state"]);
    if (query.find("notstate") != query.end())
        stateFilter->EnableNegative(query["notstate"]);
}

void TMasterHttpRequest::ServeTargetsDescriptions(IOutputStream& out0){
    const TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TBufferedOutput out(&out0);

    out << "<script type=\"text/javascript\">TargetDescriptions={";
    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        out << "\"" << (*target)->Name << "\":\"" << ReplaceAll((*target)->GetTargetDescription(), "\n", "\\n") << "\",";

    }
    out << "}</script>";
}

void TMasterHttpRequest::ServeTargets(IOutputStream& out0, const TString& workerHost) {
    TBufferedOutput out(&out0);

    const bool workerSpecified = !workerHost.empty();

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().end();

    if (workerSpecified) {
        worker = pool->GetWorkers().find(workerHost);

        if (worker == pool->GetWorkers().end()) {
            ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong worker host ") + EncodeXMLString(workerHost.data()));
            return;
        }
    }

    PrintSimpleHttpHeader(out, "text/html");

    FormatHeader(out, PT_TARGETS);
    ServeTargetsDescriptions(out);

    FormatTaggedSubgraphs(out, PT_TARGETS, graph->GetTaggedSubgraphs());

    // Filter
    TStateFilter stateFilter;
    if (Query.find("state") != Query.end())
        stateFilter.Enable(Query["state"]);
    if (Query.find("notstate") != Query.end())
        stateFilter.EnableNegative(Query["notstate"]);

    if (workerSpecified) {
        out << "<h1>Worker: " << (*worker)->GetHost() << "</h1>";

        out << "<table class=\"simple\">";
        out << "<tr><td>Address:</td><td>" << (*worker)->GetHost() << ":" << (*worker)->GetPort() << "</td></tr>";
        out << "<tr><td>State:</td><td><span id=\"$S\"></span></td></tr>";
        out << "<tr><td>Last failure:</td><td id=\"$F\"></td></tr>";
        out << "<tr><td>Reason:</td><td id=\"$R\"></td></tr>";
        out << "<tr><td>Links:</td><td>";
        out << "<a href=\"" << UrlRoot << "graph/" << (*worker)->GetHost() << "\">Graph</a>, ";
        out << "<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/log\">Worker log</a>, ";
        out << "<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/ps\">PS</a>, ";
        out << "<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/pstree\">PSTree</a>";
        out << "</td></tr>";
        out << "</table>";
    }

    // Targets
    out << "<h1>Targets" << stateFilter.GetDescr() << "</h1>";

    out << "<table class=\"grid\" id=\"MasterElement\" stateurl=\"" << UrlRoot << "status" << UrlString << "\" urlroot=\"" << UrlRoot << "\"";
    if (workerSpecified)
        out << " worker=\"" << (*worker)->GetHost() << "\"";
    out << " timestamp=\"" << graph->GetTimestamp().MilliSeconds() << "\">";

    out << "<tr>";
    out << "<th name=\"sortctrl\" sortkey=\"number\">#</th>";
    out << "<th name=\"sortctrl\" sortkey=\"type\">Type</th>";
    out << "<th name=\"sortctrl\" sortkey=\"target\">Name</th>";
    out << "<th>Action</th>";
    out << "<th>&#9733;</th>";
    out << "<th>State</th>";
    out << "<th>Depends</th>";
    out << "<th>Followers</th>";
    if (!workerSpecified)
        out << "<th>Workers</th>";
    out << "<th>Tasks</th>";
    out << "<th>&#10033;</th>";
    out << "<th name=\"sortctrl\" sortkey=\"started\">Last started</th>";
    out << "<th name=\"sortctrl\" sortkey=\"finished\">Last finished</th>";
    out << "<th name=\"sortctrl\" sortkey=\"success\">Last success</th>";
    out << "<th name=\"sortctrl\" sortkey=\"failure\">Last failure</th>";
    if (workerSpecified) {
        out << "<th name=\"sortctrl\" sortkey=\"duract\">D act</th>";
        out << "<th name=\"sortctrl\" sortkey=\"durttl\">D ttl</th>";
    } else {
        out << "<th name=\"sortctrl\" sortkey=\"durmin\">D min</th>";
        out << "<th name=\"sortctrl\" sortkey=\"duravg\">D avg</th>";
        out << "<th name=\"sortctrl\" sortkey=\"durmax\">D max</th>";
    }
    out << "<th name=\"sortctrl\" sortkey=\"cpumax\">Cpu M</th>";
    out << "<th name=\"sortctrl\" sortkey=\"cpuavg\">Cpu A</th>";
    out << "<th name=\"sortctrl\" sortkey=\"memmax\">Mem M</th>";
    out << "<th name=\"sortctrl\" sortkey=\"memavg\">Mem A</th>";
    out << "</tr>";

    if (Query.find("graphtag") != Query.end()) {
        const TTaggedSubgraphs& taggedGraphs = graph->GetTaggedSubgraphs();
        const auto taggedGraphsIt = taggedGraphs.find(Query["graphtag"]);
        Y_VERIFY(taggedGraphsIt != taggedGraphs.end());

        size_t number = 0;
        for (const auto& subgraphs : taggedGraphsIt->second) {
            const auto& topoGraph = subgraphs->GetTopoSortedTargets();
            FormatTargetsList(out, topoGraph, workerSpecified ? *worker : nullptr, stateFilter, number);
        }
    }
    else {
        size_t number = 0;
        FormatTargetsList(out, graph->GetTargets(), workerSpecified ? *worker : nullptr, stateFilter, number);
    }

    out << "</table>";

    FormatFooter(out, PT_TARGETS);
}

inline TTargetStats GetTargetStats(TMasterGraph::TTargetsList::const_iterator target, bool workerSpecified,
    TWorkerPool::TWorkersList::const_iterator worker)
{
    if (workerSpecified) {
        TTargetByWorkerStats::THostId hostId = (*target)->Type->GetHostId((*worker)->GetHost());
        return (*target)->GetTargetStats().GetStatsForTargetForWorker(hostId);
    } else {
        if ((*target)->Type->GetTaskCount() == 1) { // Need this special case to show correct information about cycled tasks
            // (i.e. to get non fake targetStats.Stats.Repeat). This 'hack' is possible as cycle could be defined only for
            // targets that has one task (although there is no any enforcement for this AFAIK it works only this way - all
            // tasks of following targets are invalidated when cycle is repeated).
            const TMasterTaskStatus& status = (*target)->GetTasks().At(0);
            return TTargetStats::FromOnlyOneTask(status.Status);
        } else {
            return (*target)->GetTargetStats().GetStatsForTarget();
        }
    }
}

void TMasterHttpRequest::ServeTargetsStatus(IOutputStream& out0, const TString& workerHost) {
    TBufferedOutput out(&out0);

    const bool workerSpecified = !workerHost.empty();

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().end();

    if (workerSpecified) {
        worker = pool->GetWorkers().find(workerHost);

        if (worker == pool->GetWorkers().end())
            return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);
    }

    PrintSimpleHttpHeader(out, "text/plain");

    TRevision::TValue Revision = 0;
    TRevision::TValue LastRevision = 0;

    TString jsonp;

    // Revision
    if (Query.find("r") != Query.end()) {
        try {
            Revision = FromString<TRevision::TValue>(Query["r"]);
        } catch (const yexception& /*e*/) {
        }
    }

    if (Query.find("jsonp") != Query.end())
        jsonp = Query["jsonp"];

    // Filter
    TStateFilter stateFilter;
    if (Query.find("state") != Query.end())
        stateFilter.Enable(Query["state"]);
    if (Query.find("notstate") != Query.end())
        stateFilter.EnableNegative(Query["notstate"]);

    if (!jsonp.empty())
        out << jsonp << "(";

    out << "{\"$#\":" << graph->GetTimestamp().MilliSeconds();

    // Error notification
    if (MasterEnv.ErrorNotificationRevision > Revision)
        out << ",\"$!\":\"" << EscapeC(MasterEnv.GetErrorNotification()) << "\"";
    LastRevision = Max<TRevision::TValue>(LastRevision, MasterEnv.ErrorNotificationRevision);

    if (workerSpecified && (*worker)->GetRevision() > Revision) {
        out << ",\"$S\":" << static_cast<int>((*worker)->GetState());
        out << ",\"$F\":" << (*worker)->GetFailureTime().Seconds();
        out << ",\"$R\":\"" << EscapeC((*worker)->GetFailureText()) << "\"";
    }

    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        LastRevision = Max(LastRevision, (*target)->GetRevision());

        if ((*target)->GetRevision() <= Revision)
            continue;

        if (workerSpecified && !(*target)->Type->HasHostname((*worker)->GetHost())) // Skip workers not serving the target
            continue;

        TTargetStats targetStats = GetTargetStats(target, workerSpecified, worker);

        // Filter pass
        if (stateFilter.IsEnabled() && stateFilter.ShouldDrop(targetStats.Stats.Tasks)) {
            out << ",\"" << (*target)->Name << "\":null";
            continue;
        }

        out << ",\"" << (*target)->Name << "\":{";

        // Tasks
        out << "\"t\":";
        NStatsFormat::FormatJSON(targetStats.Stats.Tasks, out);

        if (!workerSpecified) {
            // Workers
            out << ",\"W\":";
            NStatsFormat::FormatJSON(targetStats.Workers, out);
        }

        // Times
        out << ",";
        NStatsFormat::FormatJSON(targetStats.Stats.Times, out);

        // Durations
        out << ",";
        if (workerSpecified) {
            NStatsFormat::FormatJSON(targetStats.Stats.AggregateDurations, out);
        } else {
            NStatsFormat::FormatJSON(targetStats.Stats.Durations, out);
        }

        // Resources
        out << ",";
        NStatsFormat::FormatJSON(targetStats.Stats.Resources, out);

        // Repeat
        out << ",\"REP\":";
        NStatsFormat::FormatJSON(targetStats.Stats.Repeat, out);

        // Cron fail status
        if ((*target)->CronFailedOnAnyHost())
            out << ", \"CronFailed\": true";
        else
            out << ", \"CronFailed\": false";

        out << "}";
    }

    out << ",\"#\":" << LastRevision << "}";

    if (!jsonp.empty())
        out << ");";
}

void TMasterHttpRequest::ServeTargetCronStatusText(IOutputStream& out0, const TString& targetName) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    const TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(targetName);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

    PrintSimpleHttpHeader(out, "text/plain");
    out << ToString((*target)->CronState());
}

void TMasterHttpRequest::ServeTargetInfo(IOutputStream& out0, const TString& targetName, TString workerHost) {
    TBufferedOutput out(&out0);

    const TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    const TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(targetName);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

    TString jsonp;

    if (Query.find("jsonp") != Query.end())
        jsonp = Query["jsonp"];

    PrintSimpleHttpHeader(out, "text/html");

    if (!jsonp.empty())
        out << jsonp << '(';

    out << '{';

    if (workerHost.empty() && (*target)->Type->GetHostCount() == 1)
        workerHost = (*target)->Type->GetSingleHost();

    if (!workerHost.empty()) {
        const TMasterTargetType::THostIdSafe workerIdSafe = (*target)->Type->GetHostIdSafe(workerHost);

        if ((*target)->Type->GetTaskCountByWorker(workerIdSafe) == 1) {
            const TMasterTargetType::TTaskId taskId = (*target)->Type->GetTaskIdByWorkerIdAndLocalTaskN(workerIdSafe, 0);
            const TMasterTaskStatus* const task = &(*target)->GetTasks().At(taskId);

            out << "\"task\":{\"worker\":\"" << workerHost << "\",\"ntask\":0,\"lastchanged\":" << task->Status.GetLastChanged() << "},";
        }
    }

    out << "\"#\":" << (*target)->GetRevision() << '}';

    if (!jsonp.empty())
        out << jsonp << ");";
}

void TMasterHttpRequest::ServeTargetsXML(IOutputStream& out0) {
    TBufferedOutput out(&out0);
    PrintSimpleHttpHeader(out, "text/xml");

    out << "<?xml version=\"1.0\"?>";
    out << "<cmstatus layout=\"targets\">";

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        // Type & name
        out << "<target type=\"" << (*target)->Type->GetName() << "\" name=\"" << (*target)->Name << "\"";

        // (target has attached mail recipients)
        if ((*target)->GetRecipients() != nullptr) {
            const TRecipients* mailRecipients = (*target)->GetRecipients()->GetMailRecipients();
            if (mailRecipients != nullptr) {
                out << " mailto=\"" << JoinSeq(",", *mailRecipients) << "\"";
            }
        }
        out << ">";

        const TTargetStats& targetStats = (*target)->GetTargetStats().GetStatsForTarget();

        // Depends
        for (TMasterTarget::TDependsList::iterator i = (*target)->Depends.begin(); i != (*target)->Depends.end(); ++i)
            out << "<depend name=\"" << i->GetTarget()->Name << "\"" << ((i->IsCrossnode()) ? " crossnode=\"yes\"" : "") << " />";

        // Followers
        for (TMasterTarget::TDependsList::iterator i = (*target)->Followers.begin(); i != (*target)->Followers.end(); ++i)
            out << "<follower name=\"" << i->GetTarget()->Name << "\"" << ((i->IsCrossnode()) ? " crossnode=\"yes\"" : "") << " />";

        // Tasks
        out << "<taskstates>";
        NStatsFormat::FormatXML(targetStats.Stats.Tasks, out);
        out << "</taskstates>";

        // Times
        NStatsFormat::FormatXML(targetStats.Stats.Times, out);

        // Durations
        if (targetStats.Stats.Durations.Maximum > 0)
            NStatsFormat::FormatXML(targetStats.Stats.Durations, out);

        out << "</target>";
    }

    out << "</cmstatus>";
}

void TMasterHttpRequest::ServeTargetsText(IOutputStream& out0, const TString& workerHost) {
    TBufferedOutput out(&out0);

    const bool workerSpecified = !workerHost.empty();

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().end();

    if (workerSpecified) {
        worker = pool->GetWorkers().find(workerHost);

        if (worker == pool->GetWorkers().end())
            return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);
    }

    PrintSimpleHttpHeader(out, "text/plain");

    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        if (workerSpecified && !(*target)->Type->HasHostname((*worker)->GetHost())) // Skip workers not serving the target
            continue;

        TTargetStats targetStats = GetTargetStats(target, workerSpecified, worker);

        out << (*target)->Type->GetName() << " " << (*target)->Name;

        // Tasks
        out << " ";
        NStatsFormat::FormatText(targetStats.Stats.Tasks, out);

        // Times
        out << " ";
        NStatsFormat::FormatText(targetStats.Stats.Times, out);

        // Durations
        out << " ";
        if (workerSpecified) {
            NStatsFormat::FormatText(targetStats.Stats.AggregateDurations, out);
        } else {
            NStatsFormat::FormatText(targetStats.Stats.Durations, out);
        }

        out << "\n";
    }
}


TVector<TString> TaskPath(const TMasterTarget::TConstTaskIterator& task)
{
    const TVector<TString>& path = task.GetPath();
    Y_VERIFY(path.size() >= 1, "path must include host");
    return path;
}

void ServeOneTaskText(IOutputStream& out, const TMasterGraph::TTarget& target, const TMasterTarget::TConstTaskIterator& task)
{
    out << target.Type->GetName() << '\t';
    out << target.Name << '\t';
    out << JoinSeq(",", TaskPath(task)) << '\t';

    NStatsFormat::FormatText(TStateStats(task->Status), out);
    out << '\t';

    NStatsFormat::FormatText(TTimeStats(task->Status), out);
    out << '\t';

    out << task->Status.GetLastDuration() << '\t';
    out << task->Status.GetLastChanged() << '\n';
}

NJson::TJsonValue ServeOneTaskJsonShort(const TMasterGraph::TTarget& target, const TMasterTarget::TConstTaskIterator& task)
{
    NJson::TJsonValue val(NJson::JSON_MAP);
    val["Name"] = target.Name;

    const TVector<TString>& path = TaskPath(task);
    val["Host"] = path[0];
    if (path.size() > 1) {
        val["Id"] = path[1];
    }

    TStateStats state(task->Status);
    val["State"] = NStatsFormat::FormatString(state);

    return val;
}

void ServeOneTaskJsonShort(IOutputStream& out, const TMasterGraph::TTarget& target, const TMasterTarget::TConstTaskIterator& task)
{
    out << ServeOneTaskJsonShort(target, task) << '\n';
}

void ServeOneTaskJson(IOutputStream& out, const TMasterGraph::TTarget& target, const TMasterTarget::TConstTaskIterator& task)
{
    NJson::TJsonValue val = ServeOneTaskJsonShort(target, task);
    val["Type"] = target.Type->GetName();

    TTimeStats times(task->Status);
    val["LastStarted"] = times.LastStarted;
    val["LastFinished"] = times.LastFinished;
    val["LastSuccess"] = times.LastSuccess;
    val["LastFailure"] = times.LastFailure;

    val["LastDuration"] = task->Status.GetLastDuration();
    val["LastChanged"] = task->Status.GetLastChanged();

    if (target.GetRestartOnSuccessSchedule()) {
        val["RestartOnSuccess"] = TStringBuilder() << *target.GetRestartOnSuccessSchedule();
        val["RestartOnSuccessEnabled"] = target.GetRestartOnSuccessEnabled();
    }

    if (target.GetRetryOnFailureSchedule()) {
        val["RetryOnFailure"] = TStringBuilder() << *target.GetRetryOnFailureSchedule();
        val["RetryOnFailureEnabled"] = target.GetRetryOnFailureEnabled();
    }

    out << val << '\n';
}

void TMasterHttpRequest::ServeTasksOneTarget(IOutputStream& out, const TMasterGraph::TTarget& target,
        const TWorker* worker, const TStateFilter& stateFilter, ETaskInfoFormat format)
{
    if (worker && !target.Type->HasHostname(worker->GetHost()))
        return;

    auto task = worker ? target.TaskIteratorForHost(worker->GetHost()) : target.TaskIterator();
    while (task.Next()) {
        if (!stateFilter.ShouldDrop(task->Status)) {
            switch (format) {
                case ETaskInfoFormat::TIF_JSON:
                    ServeOneTaskJson(out, target, task);
                    break;
                case ETaskInfoFormat::TIF_JSON_SHORT:
                    ServeOneTaskJsonShort(out, target, task);
                    break;
                case ETaskInfoFormat::TIF_TEXT:
                    ServeOneTaskText(out, target, task);
                    break;
            }
        }
    }
}

void TMasterHttpRequest::ServeTasks(IOutputStream& out0, ETaskInfoFormat format) {
    TBufferedOutput out(&out0);

    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());
    const TWorker* worker = nullptr;
    if (Query.find("worker") != Query.end()) {
        const TString& workerHost = Query["worker"];

        auto workerIter = pool->GetWorkers().find(workerHost);
        if (workerIter == pool->GetWorkers().end())
            return ServeSimpleStatus(out, HTTP_NOT_FOUND);

        worker = *workerIter;
    }

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    const TMasterGraph::TTarget* oneTarget = nullptr;
    if (Query.find("target") != Query.end()) {
        const TString& targetName = Query["target"];

        auto targetIter = graph->GetTargets().find(targetName);
        if (targetIter == graph->GetTargets().end())
            return ServeSimpleStatus(out, HTTP_NOT_FOUND);

        oneTarget = *targetIter;
    }

    TStateFilter stateFilter;
    EnableStateFilterByQuery(&stateFilter, Query);

    PrintSimpleHttpHeader(out, MimeTypeByFormat(format));

    if (oneTarget) {
        ServeTasksOneTarget(out, *oneTarget, worker, stateFilter, format);
    } else {
        for (const auto& target : graph->GetTargets()) {
            ServeTasksOneTarget(out, *target, worker, stateFilter, format);
        }
    }
}

void TMasterHttpRequest::ServeTargetsStatusText(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TStateStats states;

    for (const auto& target : graph->GetTargets()) {
        for (auto task = target->TaskIterator(); task.Next(); ) {
            states.Update(task->Status);
        }
    }

    PrintSimpleHttpHeader(out, "text/plain");

    NStatsFormat::FormatText(states, out);
}

void TMasterHttpRequest::ServeTargetStatusText(IOutputStream& out0, const TString& name) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(name);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_NOT_FOUND);

    TStateStats states;

    for (TMasterTarget::TTaskIterator task = (*target)->TaskIterator(); task.Next(); ) {
        TTaskStatus status = task->Status;
        status.SetState(TTaskState(status.GetState(), 0)); // for backward compatibility, some scripts parse just state
        states.Update(status);
    }

    PrintSimpleHttpHeader(out, "text/plain");

    NStatsFormat::FormatText(states, out);
}

void TMasterHttpRequest::ServeTargetStatusText(IOutputStream& out0, const TString& name, const TString& workerName) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(name);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

    TStateStats states;

    UpdateStatsByTasks(&states, (*target)->TaskIteratorForHost(workerName));

    PrintSimpleHttpHeader(out, "text/plain");

    NStatsFormat::FormatText(states, out);
}

void TMasterHttpRequest::ServeTaskStatusText(IOutputStream& out0, const TString& name, ui32 taskId) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(name);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_NOT_FOUND);

    TStateStats states;

    for (TMasterTarget::TTaskIterator task = (*target)->TaskIterator(); task.Next(); ) {
        if (taskId == 0) {
            TTaskStatus status = task->Status;
            status.SetState(TTaskState(status.GetState(), 0)); // for backward compatibility, some scripts parse just state
            states.Update(status);
        }
        --taskId;
    }

    PrintSimpleHttpHeader(out, "text/plain");

    NStatsFormat::FormatText(states, out);
}

void TMasterHttpRequest::ServeTargetPathStatusText(IOutputStream& out0, const TString& name, TMaybe<ui32> taskId) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator bottom_target = graph->GetTargets().find(name);

    if (bottom_target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_NOT_FOUND);

    NGatherSubgraph::TResult<TMasterTarget> tempSubgraph;
    NGatherSubgraph::TMask mask(&(*bottom_target)->Type->GetParameters());

    if (taskId.Defined()) {
        if (taskId.GetRef() >= mask.Size())
            return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

        mask.At(taskId.GetRef()) = true;
    }

    NGatherSubgraph::GatherSubgraph<TMasterGraphTypes, TParamsByTypeOrdinary, TMasterGraph::TMasterTargetEdgeInclusivePredicate>
            (**bottom_target, mask, NGatherSubgraph::M_COMMAND_RECURSIVE_UP, &tempSubgraph);

    TStateStats states;

    for (const auto [target, result] : tempSubgraph.GetResultByTarget()) {
        for (auto task = target->TaskIterator(); task.Next(); ) {
            if (taskId.Empty() || result->Mask.At(task.GetN())) {
                states.Update(task->Status);
            }
        }
    }

    PrintSimpleHttpHeader(out, "text/plain");

    NStatsFormat::FormatText(states, out);
}

void TMasterHttpRequest::ServeTargetTimes(IOutputStream& out0, const TString& name) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(name);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

    TTimeStats times;

    for (TMasterTarget::TTaskIterator task = (*target)->TaskIterator(); task.Next(); ) {
        times.Update(task->Status);
    }

    PrintSimpleHttpHeader(out, "text/plain");

    out << "started=" << times.LastStarted;
    out << ",";
    out << "finished=" << times.LastFinished;
    out << ",";
    out << "success=" << times.LastSuccess;
    out << ",";
    out << "failure=" << times.LastFailure;
}


void TMasterHttpRequest::ServeDump(IOutputStream& out0) {
    TBufferedOutput out(&out0);
    PrintSimpleHttpHeader(out, "text/plain");

    TLockableHandle<TMasterGraph>::TReadLock graph(GetGraph());
    graph->DumpState(out);
}

static void GatherFollowerNames(const TMasterTarget* target, TMasterTarget::TTraversalGuard& guard, TSet<TString>& names) {
    if (!guard.TryVisit(target))
        return;

    names.insert(target->Name);

    for (TMasterTarget::TDependsList::const_iterator i = target->Followers.begin(); i != target->Followers.end(); ++i)
        GatherFollowerNames(i->GetTarget(), guard, names);
}

static void GatherDependNames(const TMasterTarget* target, TMasterTarget::TTraversalGuard& guard, TSet<TString>& names) {
    if (!guard.TryVisit(target))
        return;

    names.insert(target->Name);

    for (TMasterTarget::TDependsList::const_iterator i = target->Depends.begin(); i != target->Depends.end(); ++i)
        GatherDependNames(i->GetTarget(), guard, names);
}

void TMasterHttpRequest::ServeTargetList(IOutputStream& out0, const TVector<TString>& targets) {
    TBufferedOutput out(&out0);
    PrintSimpleHttpHeader(out, "text/plain");

    for (const TString& target : targets) {
        out << target << "\n";
    }
}

void TMasterHttpRequest::ServeTargetFollowers(IOutputStream& out, const TString& top) {
    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator top_target = graph->GetTargets().find(top);

    if (top_target == graph->GetTargets().end())
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST, "Target '" + top + "' not found.");

    TSet<TString> followers;
    {
        TMasterTarget::TTraversalGuard guard;
        GatherFollowerNames(*top_target, guard, followers);
    }

    ServeTargetList(out, TVector<TString>(followers.begin(), followers.end()));
}

void TMasterHttpRequest::ServeTargetDependencies(IOutputStream& out, const TString& bottom) {
    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator bottom_target = graph->GetTargets().find(bottom);

    if (bottom_target == graph->GetTargets().end())
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST, "Target '" + bottom + "' not found.");

    TSet<TString> depends;
    {
        TMasterTarget::TTraversalGuard guard;
        GatherDependNames(*bottom_target, guard, depends);
    }

    ServeTargetList(out, TVector<TString>(depends.begin(), depends.end()));
}

// gets list of targets which are both (indirect) followers of target `top' and (indirect) depends of
// target `bottom'. E.g. targets which will be run after `invalidate top recursive-down' + `run bottom recursive-up'
void TMasterHttpRequest::ServeTargetsDiamond(IOutputStream& out, const TString& top, const TString& bottom) {
    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator top_target = graph->GetTargets().find(top);
    TMasterGraph::TTargetsList::const_iterator bottom_target = graph->GetTargets().find(bottom);

    if (top_target == graph->GetTargets().end())
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST, "Target '" + top + "' not found.");
    if (bottom_target == graph->GetTargets().end())
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST, "Target '" + bottom + "' not found.");

    TSet<TString> followers;
    TSet<TString> depends;

    {
        TMasterTarget::TTraversalGuard guard;
        GatherFollowerNames(*top_target, guard, followers);
    }

    {
        TMasterTarget::TTraversalGuard guard;
        GatherDependNames(*bottom_target, guard, depends);
    }

    TVector<TString> diamondTargets;
    SetIntersection(followers.begin(), followers.end(), depends.begin(), depends.end(), std::back_inserter(diamondTargets));
    ServeTargetList(out, diamondTargets);
}

void TMasterHttpRequest::ServeTargetToggleRestartOnSuccessOrFailure(IOutputStream& out, const TString& targetName, const TString& successOrFailure) {
    TLockableHandle<TMasterGraph>::TWriteLock graph(GetGraph());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(targetName);
    if (target == graph->GetTargets().end()) {
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST);
    }

    bool onSuccessOrOnFailure;
    if (successOrFailure == "restart_on_success") {
        onSuccessOrOnFailure = true;
    } else if (successOrFailure == "retry_on_failure") {
        onSuccessOrOnFailure = false;
    } else {
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST);
    }

    if (Query.find("action") == Query.end()) {
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST);
    }
    TString action = Query["action"];
    bool newState;
    if (action == "enable") {
        newState = true;
    } else if (action == "disable") {
        newState = false;
    } else {
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST);
    }

    LOG1(http, RequesterName << ": Switching " << (onSuccessOrOnFailure ? "restart_on_success" : "retry_on_failure") << " for target " << targetName << ". Now it is " << (newState ? "enabled" : "disabled") << ".");

    if (onSuccessOrOnFailure) {
        (*target)->SetRestartOnSuccessEnabled(newState);
    } else {
        (*target)->SetRetryOnFailureEnabled(newState);
    }

    (*target)->SaveState();

    return ServeSimpleStatus(out, HTTP_NO_CONTENT);
}
