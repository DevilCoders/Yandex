#include "command_alias.h"
#include "http.h"
#include "http_common.h"
#include "master.h"
#include "master_profiler.h"
#include "master_target_graph.h"
#include "revision.h"
#include "transf_file.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/util.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/stream/pipe.h>

void TMasterHttpRequest::ServeSummary(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    PrintSimpleHttpHeader(out, "text/html");

    FormatHeader(out, PT_SUMMARY);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    out << "<table class=\"summary\" id=\"MasterElement\" stateurl=\"" << UrlRoot << "status\" timestamp=\"" << graph->GetTimestamp().MilliSeconds() << "\"><tr><td valign=\"top\">";

    out << "<h1>Workers</h1>";

    out << "<table class=\"simple\" id=\"$W\">";
    out << "</table>";

    out << "</td><td valign=\"top\">";

    out << "<h1>Targets</h1>";
    out << "<table class=\"simple\" id=\"$T\">";
    out << "</table>";

    out << "</td><td valign=\"top\">";

    out << "<h1>Tasks</h1>";
    out << "<table class=\"simple\" id=\"$t\">";
    out << "</table>";

    out << "</table>";

    out << "<h1>Master</h1>";

    out << "<table class=\"simple\">";
    if (!ReadOnly) {
        out << "<tr><td>Commands:</td><td>";
        out << "<a href=\"" << UrlRoot << "command/reload\" onclick=\"return confirm(this.innerHTML);\">Reload script</a>";
        out << "</td></tr>";
    }
    out << "<tr><td>Links:</td><td>";
    out << "<a href=\"" << UrlRoot << "log\">Master log</a>, ";
    out << "<a href=\"" << UrlRoot << "ps\">PS</a>";
    out << "</td></tr>";
    out << "<tr><td>Monitoring:</td><td>";
    out << "<a href=\"" << UrlRoot << "monitoring\" target=\"_blank\">Tasks&nbsp;by&nbsp;target</a>&nbsp;";
    out << "(<a href=\"" << UrlRoot << "monitoring?notype\" target=\"_blank\">no&nbsp;type&nbsp;column</a>) / ";
    out << "<a href=\"" << UrlRoot << "monitoring?byworker\" target=\"_blank\">Workers&nbsp;by&nbsp;target</a>&nbsp;";
    out << "(<a href=\"" << UrlRoot << "monitoring?byworker&notype\" target=\"_blank\">no&nbsp;type&nbsp;column</a>) / ";
    out << "<a href=\"" << UrlRoot << "monitoring?graphonly=1\" target=\"_blank\">Graph&nbsp;only</a>";
    out << "</td></tr>";
    out << "</table>";

    if (!ReadOnly) {
        out << "<h1>Pinned commands</h1>";

        out << "<ul class=\"pinned\">";

        for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
            if (!(*target)->GetPinneds().empty()) {
                for (TVector<TString>::const_iterator pinned = (*target)->GetPinneds().begin(); pinned != (*target)->GetPinneds().end(); ++pinned) {
                    try {
                        const TStringBuf& description = NCA::GetDescription(*pinned); // May throw NCA::TNoAlias

                        out << "<li class=\"pinned " << *pinned << "\"><a onclick=\"return confirm(this.innerHTML);\" href=\"";
                        out << UrlRoot << "command/" << *pinned << "?target=" << (*target)->Name;
                        out << "\">" << (*target)->Name << " &rarr; " << description << "</a></li>";
                    } catch (const NCA::TNoAlias&) {
                    }
                }
            }
        }

        out << "</ul>";
    }

    out << "<h1>Master version</h1>";

    out << "<pre>" << GetProgramSvnVersion() << "</pre>";

    out << "</body></html>";

    FormatFooter(out, PT_MONITORING);

    out.Flush();
}

void TMasterHttpRequest::ServeSummaryStatus(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    PrintSimpleHttpHeader(out, "text/plain");

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    int workersNDisconnected = 0;
    int workersNConnecting = 0;
    int workersNConnected = 0;
    int workersNAuthenticating = 0;
    int workersNInitializing = 0;
    int workersNActive = 0;
    int workersTotal = 0;

    for (TWorkerPool::TWorkersList::const_iterator i = pool->GetWorkers().begin(); i != pool->GetWorkers().end(); ++i) {
        switch ((*i)->GetState()) {
            case WS_DISCONNECTED:   ++workersNDisconnected; break;
            case WS_CONNECTING:     ++workersNConnecting; break;
            case WS_CONNECTED:      ++workersNConnected; break;
            case WS_AUTHENTICATING: ++workersNAuthenticating; break;
            case WS_INITIALIZING:   ++workersNInitializing; break;
            case WS_ACTIVE:         ++workersNActive; break;
        }
        ++workersTotal;
    }

    TString jsonp;

    if (Query.find("jsonp") != Query.end())
        jsonp = Query["jsonp"];

    if (!jsonp.empty())
        out << jsonp << "(";

    out << "{\"#\":0,\"$#\":" << graph->GetTimestamp().MilliSeconds();

    // Error notification
    out << ",\"$!\":\"" << EscapeC(MasterEnv.GetErrorNotification()) << "\"";

    // Workers summary
    out << ",\"$W\":{";
    out << "\"" << static_cast<int>(WS_DISCONNECTED) << "\":" << workersNDisconnected << ",";
    out << "\"" << static_cast<int>(WS_CONNECTING) << "\":" << workersNConnecting << ",";
    out << "\"" << static_cast<int>(WS_CONNECTED) << "\":" << workersNConnected << ",";
    out << "\"" << static_cast<int>(WS_AUTHENTICATING) << "\":" << workersNAuthenticating << ",";
    out << "\"" << static_cast<int>(WS_INITIALIZING) << "\":" << workersNInitializing << ",";
    out << "\"" << static_cast<int>(WS_ACTIVE) << "\":" << workersNActive << ",";
    out << "\"t\":" << workersTotal;
    out << "}";

    // targets
    TStateStats tasks;
    TStateStats targets;

    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        TStateStats stats((*target)->GetStateCounter());
        tasks.Update(stats);
        targets.UpdateAndGroup(stats);
    }

    out << ",\"$T\":";
    NStatsFormat::FormatJSON(targets, out, true);

    out << ",\"$t\":";
    NStatsFormat::FormatJSON(tasks, out, true);

    out << "}";

    if (!jsonp.empty())
        out << ");";

    out.Flush();
}

void TMasterHttpRequest::ServeLog(IOutputStream& out) {
    if (MasterOptions.LogFilePath.empty())
        return ServeSimpleStatus(out, HTTP_SERVICE_UNAVAILABLE, "Log file is not set");

    TFILEPtr in(MasterOptions.LogFilePath.data(), "r");

    PrintSimpleHttpHeader(out, "text/plain");

    TransferDataLimited(in, out, 0, 1024 * 1024);
}

void TMasterHttpRequest::ServePS(IOutputStream& out) {
    try {
        TPipeInput ps("ps auxww | sort");

        PrintSimpleHttpHeader(out, "text/html");

        out << "<html><head><title>Processlist</title></head><body><pre>\n";

        try {
            TString line;
            // Looks like pclose() on TPipeInput::DoRead will always throw an exception at the end of reading.
            while (ps.ReadLine(line)) {
                if (line.StartsWith("USER"))
                    out << "<span style=\"font-weight: bold\">";
                else
                    out << "<span>";

                out << EncodeXMLString(line.data());

                out << "</span>\n";
            }
        } catch (const yexception& /*e*/) {}

        out << "</pre></body></html>";
    } catch (const yexception& e) {
        ServeSimpleStatus(out, HTTP_INTERNAL_SERVER_ERROR, TString("Error serving PS: ") + e.what());
        return;
    }
}

void TMasterHttpRequest::ServeMonitoring(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    PrintSimpleHttpHeader(out, "text/html");

    bool notype = Query.find("notype") != Query.end();

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    FormatHeader(out, PT_MONITORING);

    out << "<table id=\"MasterElement\" stateurl=\"" << UrlRoot << "status" << UrlString << "\" timestamp=\"" << graph->GetTimestamp().MilliSeconds() << (graph->GetGraphImage() ? "Y" : "N") << "\" ";
    out << "width=\"100%\" height=\"100%\" border=\"0\" cellspacin=\"0\" cellpadding=\"0\">";
    out << "<tr><td align=\"left\" valign=\"middle\">";

    out << "<table id=\"Targets\" class=\"grid\">";
    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        out << "<tr style=\"display:none\">";
        if (!notype)
            out << "<td class=\"type\">" << (*target)->Type->GetName() << "</td>";
        out << "<td>" << (*target)->Name << "</td><td name=\"@X\" id=\"" << (*target)->Name << "\"></td></tr>";
    }
    out << "<tr>" << (notype ? "" : "<td>&nbsp;</td>") << "<td>&nbsp;</td><td>&nbsp;</td></td></table>";

    out << "</td><td align=\"right\" valign=\"middle\">";

    out << "<object id=\"Graph\" type=\"image/svg+xml\" data=\"" << UrlRoot << "graph.svg\" height=\"100%\">";

    out << "</td></tr></table>";

    FormatFooter(out, PT_MONITORING);

    out.Flush();
}

void TMasterHttpRequest::ServeMonitoringStatus(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    PrintSimpleHttpHeader(out, "text/plain");

    bool byworker = Query.find("byworker") != Query.end();

    TRevision::TValue Revision = 0;
    TRevision::TValue LastRevision = 0;

    if (Query.find("r") != Query.end()) {
        try {
            Revision = FromString<TRevision::TValue>(Query["r"]);
        } catch (const yexception& /*e*/) {
        }
    }

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    out << "{\"$#\":\"" << graph->GetTimestamp().MilliSeconds() << (graph->GetGraphImage() ? "Y\"" : "N\"");

    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        LastRevision = Max(LastRevision, (*target)->GetRevision());

        if ((*target)->GetRevision() <= Revision)
            continue;

        TStateStats stats;

        if (byworker) {
            for (TMasterTargetType::THostId hostId = 0; hostId < (*target)->Type->GetHostCount(); ++hostId) {
                TStateStats workerStats;
                UpdateStatsByTasks(&workerStats, (*target)->TaskIteratorForHost(hostId));
                stats.UpdateAndGroup(workerStats);
            }
        } else {
            UpdateStatsByTasks(&stats, (*target)->TaskIterator());
        }

        // Tasks
        out << ",\"" << (*target)->Name << "\":";
        NStatsFormat::FormatJSON(stats, out);
    }

    out << ",\"#\":" << LastRevision << "}";

    out.Flush();
}

TString FormatTimeCounter(long value, long durationSeconds) {
    TDuration valueDuration = TDuration::MicroSeconds(value);
    TDuration duration = TDuration::Seconds(durationSeconds);

    double percent = ((double) valueDuration.MicroSeconds() / duration.MicroSeconds()) * 100;
    if (percent < 1) {
        TStringStream ss;
        if (valueDuration.Seconds() > 0) {
            ss << valueDuration.Seconds() << "s";
        } else if (valueDuration.MilliSeconds() > 0) {
            ss << valueDuration.MilliSeconds() << "ms";
        } else {
            ss << valueDuration.MicroSeconds() << "mis";
        }
        return ss.Str();
    } else {
        double percentRounded = (percent > 100 && percent < 105) ? 100 : percent; // round values that are slightly greater than 100 (because of measurement inaccuracy)
        return Sprintf("%0.2f%%", percentRounded);
    }
}

template<class T>
void FormatOrdinaryCountersString(T id, const TInstant& now, const TTimeCounters<T>& counters, IOutputStream& out) {
    out << counters.GetNameById(id) << "\t" << counters.Get15Seconds(id, now) << "\t"
            << counters.Get30Seconds(id, now) << "\t" << counters.GetOneMinute(id, now) << "\t"
            << counters.Get5Minutes(id, now) << "\t" << counters.GetHalfAnHour(id, now) << "\n";
}

enum ECountersGroup {
    CG_ALL,
    CG_MAIN_LOOP,
    CG_NETWORK_MESSAGES,
    CG_HTTP
};

template <>
inline ECountersGroup FromString<ECountersGroup>(const TString& s) {
    if (s == "all")              return CG_ALL;
    if (s == "main_loop")        return CG_MAIN_LOOP;
    if (s == "network_messages") return CG_NETWORK_MESSAGES;
    if (s == "http")             return CG_HTTP;
    ythrow yexception() << "bad counters group";
}

void TMasterHttpRequest::ServeCountersText(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    PrintSimpleHttpHeader(out, "text/plain");

    ECountersGroup group = CG_ALL;
    if (Query.find("type") != Query.end()) {
        try {
            group = FromString<ECountersGroup>(Query["type"]);
        } catch (const yexception& /*e*/) {
        }
    }

    TInstant now = TInstant::Now();

    typedef TVector<TMasterProfiler::EMasterProfilerId> TIds;

    out << "Counter\t15s\t30s\t1m\t5m\t30m\n";
    if (group == CG_MAIN_LOOP || group == CG_ALL) {
        for (TIds::const_iterator i = GetMasterProfiler().MainLoopTimeCounterIds.begin(); i != GetMasterProfiler().MainLoopTimeCounterIds.end(); i++) {
            out << GetMasterProfiler().Counters->GetNameById(*i) << "\t" << FormatTimeCounter(GetMasterProfiler().Counters->Get15Seconds(*i, now), 15)  << "\t"
                    << FormatTimeCounter(GetMasterProfiler().Counters->Get30Seconds(*i, now), 30) << "\t" << FormatTimeCounter(GetMasterProfiler().Counters->GetOneMinute(*i, now), 60) << "\t"
                    << FormatTimeCounter(GetMasterProfiler().Counters->Get5Minutes(*i, now), 300) << "\t" << FormatTimeCounter(GetMasterProfiler().Counters->GetHalfAnHour(*i, now), 1800) << "\n";
        }
        TMasterProfiler::EMasterProfilerId ml = TMasterProfiler::ID_MAIN_LOOP_ITERATIONS;
        FormatOrdinaryCountersString(ml, now, *GetMasterProfiler().Counters, out);
    }
    if (group == CG_NETWORK_MESSAGES || group == CG_ALL) {
        const TTimeCounters<int>& msgCounters = *GetMessageProfiler2().Counters;
        for (TTimeCounters<int>::TNamesVector::const_iterator i = msgCounters.GetNames().begin(); i != msgCounters.GetNames().end(); i++) {
            FormatOrdinaryCountersString(i->Id, now, msgCounters, out);
        }
    }
    if (group == CG_HTTP || group == CG_ALL) {
        for (TIds::const_iterator i = GetMasterProfiler().HttpCounterIds.begin(); i != GetMasterProfiler().HttpCounterIds.end(); i++) {
            FormatOrdinaryCountersString(*i, now, *GetMasterProfiler().Counters, out);
        }
    }

    out.Flush();
}
