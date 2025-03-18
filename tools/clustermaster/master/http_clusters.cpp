#include "http.h"
#include "http_common.h"
#include "master.h"
#include "master_target_graph.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/util.h>

#include <util/generic/utility.h>

void FormatLogLinks(TBufferedOutput& out, ui8 numberOfPreviousTargetLogs, const TString& UrlRoot, const TString& workerHost, const TString& targetName, int taskNumber) {
    TStringBuilder logLinks;
    TStringBuilder fulllogLinks;
    fulllogLinks << "&nbsp;";
    for (int i = 0; i <= numberOfPreviousTargetLogs; ++i) {
        if (i == 1) {
            logLinks << "&nbsp;[";
            fulllogLinks << "&nbsp;[";
        } else if (i > 1) {
            logLinks << "&nbsp;";
            fulllogLinks << "&nbsp;";
        }

        logLinks << "<a href=\"" << UrlRoot << "proxy/" << workerHost << "/logs/" << targetName << "/" << taskNumber;
        fulllogLinks << "<a href=\"" << UrlRoot << "proxy/" << workerHost << "/fullogs/" << targetName << "/" << taskNumber;
        TString logLinkName;
        TString fulllogLinkName;

        if (i != 0) {
            logLinks<< "/" << i;
            logLinkName = ToString(i);
            fulllogLinks << "/" << i;
            fulllogLinkName = ToString(i);
        } else {
            logLinkName = "Log";
            fulllogLinkName = "Full log";
        }

        logLinks << "\" title=\"Show log\">" << logLinkName << "</a>";
        fulllogLinks << "\" title=\"Show full log\">" << fulllogLinkName << "</a>";

    }

    if (numberOfPreviousTargetLogs > 0) {
        logLinks << "]";
        fulllogLinks << "]";
    }

    out << logLinks;
    out << fulllogLinks;
}

void TMasterHttpRequest::ServeClusters(IOutputStream& out0, const TString& workerHost, const TString& targetName) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().find(workerHost);
    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(targetName);

    if (worker == pool->GetWorkers().end()) {
        ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong worker host ") + EncodeXMLString(workerHost.data()));
        return;
    }

    if (target == graph->GetTargets().end()) {
        ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong target name ") + EncodeXMLString(targetName.data()));
        return;
    }

    PrintSimpleHttpHeader(out, "text/html");

    FormatHeader(out, PT_CLUSTERS);

    out << "<h1>Target " << (*target)->Name << " on worker " << (*worker)->GetHost() << "</h1>";

    out << "<table class=\"simple\" id=\"$t\">";
    out << "</table>";

    // Process filter
    TStateFilter stateFilter;
    if (Query.find("state") != Query.end())
        stateFilter.Enable(Query["state"]);
    if (Query.find("notstate") != Query.end())
        stateFilter.EnableNegative(Query["notstate"]);

    ui32 paramCount = (*target)->Type->GetParamCount();
    ui32 valueColumnCount = Max(ui32(1), paramCount);

    // Summary
    out << "<h1>Tasks" << stateFilter.GetDescr() << "</h1>";

    out << "<table class=\"grid\" id=\"MasterElement\" stateurl=\"" << UrlRoot << "status" << UrlString << "\" urlroot=\"" << UrlRoot << "\" worker=\"" << (*worker)->GetHost() << "\" target=\"" << (*target)->Name << "\" timestamp=\"" << graph->GetTimestamp().MilliSeconds() << "\">";
    out << "<tr>";
    out << "<th name=\"sortctrl\" sortkey=\"number\">#</th>";
    for (ui32 i = 0; i < valueColumnCount; ++i) {
        out << "<th name=\"sortctrl\" sortkey=\"value\">Value</th>";
    }
    out << "<th>&#9733;</th>";
    out << "<th>State</th>";
    out << "<th>Log</th>";
    out << "<th>&#10033;</th>";
    out << "<th name=\"sortctrl\" sortkey=\"started\">Last started</th>";
    out << "<th name=\"sortctrl\" sortkey=\"finished\">Last finished</th>";
    out << "<th name=\"sortctrl\" sortkey=\"success\">Last success</th>";
    out << "<th name=\"sortctrl\" sortkey=\"failure\">Last failure</th>";
    out << "<th name=\"sortctrl\" sortkey=\"duration\">Last duration</th>";
    out << "<th name=\"sortctrl\" sortkey=\"cpumax\">Cpu M</th>";
    out << "<th name=\"sortctrl\" sortkey=\"cpuavg\">Cpu A</th>";
    out << "<th name=\"sortctrl\" sortkey=\"memmax\">Mem M</th>";
    out << "<th name=\"sortctrl\" sortkey=\"memavg\">Mem A</th>";
    out << "</tr>";

    // Table
    int ntask = 0;
    for (TMasterTarget::TTaskIterator task = (*target)->TaskIteratorForHost(workerHost); task.Next(); ++ntask) {
        TVector<TString> path = task.GetPath();
        Y_VERIFY(path.size() >= 1, "path must at least include host");

        TString firstParamName = path.size() == 1 ? NO_CLUSTERS_NAME : path.at(1);

        out << "<tr name=\"@@\" task=\"" << ntask << "\" value=\"" << firstParamName << "\"";
        if (stateFilter.IsEnabled())
            out << " style=\"display:none\"";
        out << ">";

        out << "<td class=\"number\">" << ntask << "</td>";

        out << "<td>" << firstParamName << "</td>\n";
        for (size_t i = 2; i < path.size(); ++i) {
            out << "<td>" << path.at(i) << "</td>\n";
        }

        // Communism
        out << "<td class=\"smarthint\" name=\"SH\" hinturl=\"" << UrlRoot << "proxy/" << (*worker)->GetHost() << "/res/" << (*target)->Name << '/' << ntask << "\">&#9733;</td>";

        // State
        out << "<td name=\"@X\"></td>";

        // Log
        out << "<td>";
        FormatLogLinks(out, MasterOptions.NTargetLogs, UrlRoot, (*worker)->GetHost(), (*target)->GetName(), ntask);
        out << "</td>";

        // Info
        out << "<td class=\"smarthint\" name=\"IH\" hinturl=\"" << UrlRoot << "info" << UrlString << '/' << ntask << "\">&#10033;</td>";

        // Times
        out << "<td name=\"@s\"></td>";
        out << "<td name=\"@f\"></td>";
        out << "<td name=\"@S\"></td>";
        out << "<td name=\"@F\"></td>";

        // Duration
        out << "<td name=\"@d\"></td>";

        // Resource usage statistics
        out << "<td name=\"@cM\"></td>";
        out << "<td name=\"@ca\"></td>";
        out << "<td name=\"@mM\"></td>";
        out << "<td name=\"@ma\"></td>";

        out << "</tr>";
    }

    out << "</table>";

    FormatFooter(out, PT_CLUSTERS);

    out.Flush();
}

void TMasterHttpRequest::ServeClustersStatus(IOutputStream& out0, const TString& workerHost, const TString& targetName) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().find(workerHost);
    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(targetName);

    if (worker == pool->GetWorkers().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

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

    // Process filter
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

    LastRevision = Max((*worker)->GetRevision(), (*target)->GetRevision());
    if ((*worker)->GetRevision() > Revision && (*target)->GetRevision() > Revision) {

        // Summary
        TStateStats states;
        UpdateStatsByTasks(&states, (*target)->TaskIterator());

        out << ",\"$t\":";
        NStatsFormat::FormatJSON(states, out, true);

        int ntask = 0;
        for (TMasterTarget::TTaskIterator task = (*target)->TaskIteratorForHost(workerHost); task.Next(); ++ntask) {
            if (stateFilter.ShouldDrop(task->Status)) {
                out << "," << ntask << ":null";
                continue;
            }

            out << "," << ntask << ":{";

            TStateStats state(task->Status);
            TTimeStats times(task->Status);
            TResStats resources(task->Status);
            TRepeatStats repeater(task->Status);

            // State
            out << "\"t\":";
            NStatsFormat::FormatJSON(state, out);

            // Times
            out << ",";
            NStatsFormat::FormatJSON(times, out);

            // Duration
            out << ",\"d\":" << task->Status.GetLastDuration();

            // Resources
            out << ",";
            NStatsFormat::FormatJSON(resources, out);

            // Repeat
            out << ",\"REP\":";
            NStatsFormat::FormatJSON(repeater, out);

            out << "}";
        }
    }

    out << ",\"#\":" << LastRevision << "}";

    if (!jsonp.empty())
        out << ");";

    out.Flush();
}

void TMasterHttpRequest::ServeClusterInfo(IOutputStream& out0, const TString& workerHost, const TString& targetName, const TString& taskNumber) {
    TBufferedOutput out(&out0);

    const ui32 nTask = FromString<ui32>(taskNumber);

    const TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    const TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(targetName);

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);

    const TMasterTargetType::THostIdSafe workerIdSafe = (*target)->Type->GetHostIdSafe(workerHost);
    const TMasterTargetType::TTaskId taskId = (*target)->Type->GetTaskIdByWorkerIdAndLocalTaskN(workerIdSafe, nTask);
    const TMasterTaskStatus* const task = &(*target)->GetTasks().At(taskId);

    TString jsonp;

    if (Query.find("jsonp") != Query.end())
        jsonp = Query["jsonp"];

    PrintSimpleHttpHeader(out, "text/plain");

    if (!jsonp.empty())
        out << jsonp << '(';

    out << "{\"worker\":\"" << workerHost << "\",\"ntask\":" << taskNumber << ",\"lastchanged\":" << task->Status.GetLastChanged() << '}';

    if (!jsonp.empty())
        out << jsonp << ");";

    out.Flush();
}

// mostly copy-paste, sorry
void TMasterHttpRequest::ServeClustersText(IOutputStream& out0, const TString& workerHost, const TString& targetName) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().find(workerHost);
    TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().find(targetName);

    if (worker == pool->GetWorkers().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong worker host ") + EncodeXMLString(workerHost.data()));

    if (target == graph->GetTargets().end())
        return ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong target name ") + EncodeXMLString(targetName.data()));

    PrintSimpleHttpHeader(out, "text/plain");

    // Table
    for (TMasterTarget::TConstTaskIterator task = (*target)->TaskIteratorForHost(workerHost); task.Next(); ) {
        const TVector<TString>& path = task.GetPath();
        Y_VERIFY(path.size() >= 1, "path must include host");
        if (path.size() == 1) {
            out << NO_CLUSTERS_NAME;
        } else {
            for (size_t i = 1; i < path.size(); ++i) {
                if (i != 1)
                    out << ",";
                out << path.at(i);
            }
        }

        TStateStats state(task->Status);
        TTimeStats times(task->Status);

        // State
        out << "\t";
        NStatsFormat::FormatText(state, out);

        // Times
        out << "\t";
        NStatsFormat::FormatText(times, out);

        // Duration
        out << "\t" << task->Status.GetLastDuration() << "\n";
    }

    out.Flush();
}

