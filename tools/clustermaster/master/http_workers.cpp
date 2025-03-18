#include "cron_state.h"
#include "http.h"
#include "http_common.h"
#include "master.h"
#include "master_target_graph.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/util.h>

#include <iomanip>

void TMasterHttpRequest::ServeWorkers(IOutputStream& out0, const TString& targetName) {
    TBufferedOutput out(&out0);

    const bool targetSpecified = !targetName.empty();

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().end();

    if (targetSpecified) {
        target = graph->GetTargets().find(targetName);

        if (target == graph->GetTargets().end()) {
            ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong target name ") + EncodeXMLString(targetName.data()));
            return;
        }
    }

    PrintSimpleHttpHeader(out, "text/html");

    // Filter
    TStateFilter stateFilter;
    if (Query.find("state") != Query.end())
        stateFilter.Enable(Query["state"]);
    if (Query.find("notstate") != Query.end())
        stateFilter.EnableNegative(Query["notstate"]);

    FormatHeader(out, PT_WORKERS);
    ServeTargetsDescriptions(out);

    if (targetSpecified)
        out << "<h1>Target: " << (*target)->Name << "</h1>";

    out << "<h1>Workers" << stateFilter.GetDescr() << "</h1>";

    out << "<table class=\"grid\" id=\"MasterElement\" stateurl=\"" << UrlRoot << "status" << UrlString << "\" urlroot=\"" << UrlRoot << "\"";
    if (targetSpecified)
        out << " target=\"" << (*target)->Name << "\"";
    out << " timestamp=\"" << graph->GetTimestamp().MilliSeconds() << "\">";

    out << "<tr>";
    out << "<th name=\"sortctrl\" sortkey=\"number\">#</th>";
    if (targetSpecified)
        out << "<th>C</th>";
    out << "<th>G</th>";
    out << "<th name=\"sortctrl\" sortkey=\"worker\">Host</th>";
    if (!targetSpecified) {
        out << "<th>Connection</th>";
        out << "<th name=\"sortctrl\" sortkey=\"diskspace\">Free</th>";
    }
    out << "<th>Action</th>";
    out << "<th>&#9733;</th>";
    out << "<th>State</th>";
    if (!targetSpecified)
        out << "<th>Targets</th>";
    out << "<th>Tasks</th>";
    out << "<th name=\"sortctrl\" sortkey=\"started\">Last started</th>";
    out << "<th name=\"sortctrl\" sortkey=\"finished\">Last finished</th>";
    out << "<th name=\"sortctrl\" sortkey=\"success\">Last success</th>";
    out << "<th name=\"sortctrl\" sortkey=\"failure\">Last failure</th>";
    if (targetSpecified) {
        out << "<th name=\"sortctrl\" sortkey=\"duract\">D act</th>";
        out << "<th name=\"sortctrl\" sortkey=\"durttl\">D ttl</th>";
    }
    out << "<th name=\"sortctrl\" sortkey=\"cpumax\">Cpu M</th>";
    out << "<th name=\"sortctrl\" sortkey=\"cpuavg\">Cpu A</th>";
    out << "<th name=\"sortctrl\" sortkey=\"memmax\">Mem M</th>";
    out << "<th name=\"sortctrl\" sortkey=\"memavg\">Mem A</th>";
    out << "<th>Last messsage</th>";
    out << "</tr>";

    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
        bool singletask = false;

        if (targetSpecified) {
            // Skip workers not serving the targets
            if (!(*target)->Type->HasHostname((*worker)->GetHost())) {
                continue;
            }

            singletask = (*target)->Type->GetTaskCountByWorker((*worker)->GetHost()) == 1;
        }

        const size_t number = worker - pool->GetWorkers().begin();

        out << "<tr name=\"@@\" number=\"" << number << "\"worker=\"" << (*worker)->GetHost() << "\"";
        if (stateFilter.IsEnabled())
            out << " style=\"display:none\"";
        out << ">";

        out << "<td class=\"number\">" << number << "</td>";

        if (targetSpecified)
            out << "<td name=\"@C\"></td>";

        if ((*worker)->GetGroupId() != -1)
            out << "<td>" << (*worker)->GetGroupId() << "</td>";
        else
            out << "<td>&nbsp;</td>";

        // Name
        out << "<td>";
        if (targetSpecified) {
            out << "<a href=\"" << UrlRoot << "target/" << (*target)->Name << "/" << (*worker)->GetHost() << "\"";
            out << " name=\"AM\" cmdparams=\"target=" << (*target)->Name << "&worker=" << (*worker)->GetHost() << "\">" << (*worker)->GetHost() << "</a>";
        } else {
            out << "<a href=\"" << UrlRoot << "worker/" << (*worker)->GetHost() << "\">" << (*worker)->GetHost() << "</a>";
        }
        out << "</td>";

        // Connection & Diskspace
        if (!targetSpecified) {
            out << "<td name=\"@C\"></td>";
            out << "<td style=\"text-align:center\" name=\"@D\"></td>";
        }

        // Action
        out << "<td style=\"text-align:center\">";
        out << "<a href=\"" << UrlRoot << "graph/" << (*worker)->GetHost() << "\" title=\"View graph for the worker\">Graph</a>";
        out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/log\" title=\"View worker log\">Worker log</a>";
        out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/ps\" title=\"View ps on worker\">PS</a>";
        out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/pstree\" title=\"View pstree on worker\">PSTree</a>";
        if (targetSpecified && singletask) {
            out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/logs/" << (*target)->Name << "/0\">Log</a>";
            out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/fullogs/" << (*target)->Name << "/0\">Full log</a>";
        }
        out << "&nbsp;<a href=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/dump\" title=\"View state dump\">Dump</a>";
        out << "</td>";

        // Communism
        if (targetSpecified && singletask) {
            out << "<td class=\"smarthint\" name=\"SH\" hinturl=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/res/" << (*target)->Name << "/0\">&#9733;</td>";
        } else {
            out << "<td>&nbsp;</td>";
        }

        // State
        out << "<td name=\"@X\"></td>";

        if (!targetSpecified) {
            // Targets
            out << "<td style=\"text-align:center\" name=\"@T\"></td>";
        }

        // Tasks
        out << "<td style=\"text-align:center\" name=\"@t\"></td>";

        // Times
        out << "<td name=\"@s\"></td>";
        out << "<td name=\"@f\"></td>";
        out << "<td name=\"@S\"></td>";
        out << "<td name=\"@F\"></td>";

        // Durations
        if (targetSpecified) {
            out << "<td name=\"@dA\"></td>";
            out << "<td name=\"@dt\"></td>";
        }

        // Resource usage statistics
        out << "<td name=\"@cM\"></td>";
        out << "<td name=\"@ca\"></td>";
        out << "<td name=\"@mM\"></td>";
        out << "<td name=\"@ma\"></td>";

        // Failure
        out << "<td name=\"@M\"></td>";

        out << "</tr>";

    }

    out << "</table>";

    FormatFooter(out, PT_WORKERS);

    out.Flush();
}

TWorkerStats CalculateAllTargetsWorkerStats(const TString& workerHost,
        const TLockableHandle<TMasterGraph>::TReloadScriptReadLock& graph)
{
    TWorkerStats result;

    TStatsCalculator calc;
    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        if (!(*target)->Type->HasHostname(workerHost)) {
            continue;
        }

        TTargetByWorkerStats::THostId hostId = (*target)->Type->GetHostId(workerHost);

        const TWorkerStats& workerForTargetStats = (*target)->GetTargetStats().GetStatsForWorker(hostId);

        result.Stats.Tasks.Update(workerForTargetStats.Stats.Tasks);
        result.Stats.Times.Update(workerForTargetStats.Stats.Times);
        result.Targets.UpdateAndGroup(workerForTargetStats.Stats.Tasks);

        // workerForTargetStats.Stats.AggregateDurations isn't used intentionally - result.Stats.AggregateDurations actually
        // isn't needed in markup and anyway there is no accurate way to calculate it from list of
        // workerForTargetStats.Stats.AggregateDurations
        calc.Add(workerForTargetStats.Stats.Resources, workerForTargetStats.Stats.Durations);
    }
    calc.Finalize();

    result.Stats.Resources = calc.GetResources();
    result.Stats.Durations = calc.GetDurations();
    result.Stats.AggregateDurations = calc.GetAggregateDurations();

    return result;
}

inline TWorkerStats GetWorkerStats(TWorkerPool::TWorkersList::const_iterator worker, bool targetSpecified,
    TMasterGraph::TTargetsList::const_iterator target, const TLockableHandle<TMasterGraph>::TReloadScriptReadLock& graph)
{
    if (targetSpecified) {
        TTargetByWorkerStats::THostId hostId = (*target)->Type->GetHostId((*worker)->GetHost());
        return (*target)->GetTargetStats().GetStatsForWorker(hostId);
    } else {
        return CalculateAllTargetsWorkerStats((*worker)->GetHost(), graph);
    }
}

void TMasterHttpRequest::ServeWorkersStatus(IOutputStream& out0, const TString& targetName) {
    TBufferedOutput out(&out0);

    const bool targetSpecified = !targetName.empty();

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().end();

    if (targetSpecified) {
        target = graph->GetTargets().find(targetName);

        if (target == graph->GetTargets().end())
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

    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
        LastRevision = Max(LastRevision, (*worker)->GetRevision());

        if ((*worker)->GetRevision() <= Revision)
            continue;

        if (targetSpecified && !(*target)->Type->HasHostname((*worker)->GetHost())) { // Skip workers not serving the target
            continue;
        }

        TWorkerStats workerStats = GetWorkerStats(worker, targetSpecified, target, graph);

        // Filter pass
        if (stateFilter.IsEnabled() && stateFilter.ShouldDrop(workerStats.Stats.Tasks)) {
            out << ",\"" << (*worker)->GetHost() << "\":null";
            continue;
        }

        out << ",\"" << (*worker)->GetHost() << "\":{";

        // Worker state
        out << "\"C\":" << static_cast<int>((*worker)->GetState());

        // Diskspace
        out << ",\"D\":" << (*worker)->GetAvailDiskspace();

        // Cron fail status
        if (targetSpecified) {
            if ((*target)->CronFailedOnHost((*worker)->GetHost()))
                out << ", \"CronFailed\": true";
            else
                out << ", \"CronFailed\": false";
        }

        // Tasks
        out << ",\"t\":";
        NStatsFormat::FormatJSON(workerStats.Stats.Tasks, out);

        if (!targetSpecified) {
            // Targets
            out << ",\"T\":";
            NStatsFormat::FormatJSON(workerStats.Targets, out);
        }

        // Times
        out << ",";
        NStatsFormat::FormatJSON(workerStats.Stats.Times, out);

        if (targetSpecified) {
            // Durations
            out << ",";
            NStatsFormat::FormatJSON(workerStats.Stats.AggregateDurations, out);
        }

        // Resources
        out << ",";
        NStatsFormat::FormatJSON(workerStats.Stats.Resources, out);

        // Repeat
        out << ",\"REP\":";
        NStatsFormat::FormatJSON(TRepeatStats(), out); // TODO it isn't useful to have some TRepeatStats (aggregated is some way
            // as workers nearly always has more than one task) on workers page - need check javascript and remove completely

        // Failure
        out << ",\"M\":\"" << EscapeC((*worker)->GetFailureText()) << "\"";

        out << "}";
    }

    out << ",\"#\":" << LastRevision << "}";

    if (!jsonp.empty())
        out << ");";

    out.Flush();
}

void TMasterHttpRequest::ServeWorkerInfo(IOutputStream& out0, const TString& workerHost, TString) {
    TBufferedOutput out(&out0);

    const TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    const TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().find(workerHost);

    if (worker == pool->GetWorkers().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

    TString jsonp;

    if (Query.find("jsonp") != Query.end())
        jsonp = Query["jsonp"];

    PrintSimpleHttpHeader(out, "text/html");

    if (!jsonp.empty())
        out << jsonp << '(';

    out << '{';
    out << "\"#\":" << (*worker)->GetRevision() << '}';

    if (!jsonp.empty())
        out << jsonp << ");";

    out.Flush();
}

void TMasterHttpRequest::ServeWorkersText(IOutputStream& out0, const TString& targetName) {
    TBufferedOutput out(&out0);

    const bool targetSpecified = !targetName.empty();

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().end();

    if (targetSpecified) {
        target = graph->GetTargets().find(targetName);

        if (target == graph->GetTargets().end()) {
            ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong target name ") + EncodeXMLString(targetName.data()));
            return;
        }
    }

    PrintSimpleHttpHeader(out, "text/plain");

    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
        if (targetSpecified && !(*target)->Type->HasHostname((*worker)->GetHost())) // Skip workers not serving the target
            continue;

        TWorkerStats workerStats = GetWorkerStats(worker, targetSpecified, target, graph);

        out << (*worker)->GetHost() << " " << ToString((*worker)->GetState());

        // Tasks
        out << " ";
        NStatsFormat::FormatText(workerStats.Stats.Tasks, out);

        // Times
        out << " ";
        NStatsFormat::FormatText(workerStats.Stats.Times, out);

        if (targetSpecified) {
            out << " ";
            NStatsFormat::FormatText(workerStats.Stats.AggregateDurations, out);
        }

        out << "\n";
    }

    out.Flush();
}
