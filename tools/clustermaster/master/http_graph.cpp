#include "http.h"
#include "http_common.h"
#include "master.h"
#include "master_target_graph.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/util.h>

void TMasterHttpRequest::ServeGraph(IOutputStream& out0, const TString& workerHost) {
    const bool workerSpecified = !workerHost.empty();

    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    if (workerSpecified) {
        TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

        if (pool->GetWorkers().find(workerHost) == pool->GetWorkers().end()) {
            ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong worker host ") + EncodeXMLString(workerHost.data()));
            return;
        }
    }

    PrintSimpleHttpHeader(out, "text/html");

    FormatHeader(out, PT_GRAPH);
    ServeTargetsDescriptions(out);

    FormatTaggedSubgraphs(out, PT_GRAPH, graph->GetTaggedSubgraphs());

    out << "<h1>Target graph</h1>";

    if (!MasterOptions.DisableGraphviz) {
        out << "<object id=\"MasterElement\" type=\"image/svg+xml\" data=\"" << UrlRoot << "graph.svg\" stateurl=\"" << UrlRoot << "status" << UrlString << "\" urlroot=\"" << UrlRoot << "\"";
        if (workerSpecified)
            out << " worker=\"" << workerHost << "\"";
        out << " timestamp=\"" << graph->GetTimestamp().MilliSeconds() << (graph->GetGraphImage() ? "Y" : "N") << "\">";
        out << "<span>If you cannot see the graph image, most likely graphviz is not installed on the host running master</span>";
    } else {
        out << "<span>Graph was disabled for this instance</span>";
    }

    FormatFooter(out, PT_GRAPH);
}

void TMasterHttpRequest::ServeGraphStatus(IOutputStream& out0, const TString& workerHost) {
    const bool workerSpecified = !workerHost.empty();

    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());

    if (workerSpecified) {
        TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

        if (pool->GetWorkers().find(workerHost) == pool->GetWorkers().end())
            return ServeSimpleStatus(out0, HTTP_BAD_REQUEST);
    }

    PrintSimpleHttpHeader(out, "text/plain");

    TRevision::TValue Revision = 0;
    TRevision::TValue LastRevision = 0;

    TString jsonp;

    if (Query.find("r") != Query.end()) {
        try {
            Revision = FromString<TRevision::TValue>(Query["r"]);
        } catch (const yexception& /*e*/) {
        }
    }

    if (Query.find("jsonp") != Query.end())
        jsonp = Query["jsonp"];

    if (!jsonp.empty())
        out << jsonp << "(";

    out << "{\"$#\":\"" << graph->GetTimestamp().MilliSeconds() << (graph->GetGraphImage() ? "Y\"" : "N\"");

    // Error notification
    if (MasterEnv.ErrorNotificationRevision > Revision)
        out << ",\"$!\":\"" << EscapeC(MasterEnv.GetErrorNotification()) << "\"";
    LastRevision = Max<TRevision::TValue>(LastRevision, MasterEnv.ErrorNotificationRevision);

    for (TMasterGraph::TTargetsList::const_iterator target = graph->GetTargets().begin(); target != graph->GetTargets().end(); ++target) {
        LastRevision = Max(LastRevision, (*target)->GetRevision());

        if ((*target)->GetRevision() <= Revision)
            continue;

        TStateStats states;

        if (workerSpecified) {
            if ((*target)->Type->HasHostname(workerHost)) {
                UpdateStatsByTasks(&states, (*target)->TaskIteratorForHost(workerHost));
            } else {
                out << ",\"" << (*target)->Name << "\":null";
                continue;
            }
        } else {
            UpdateStatsByTasks(&states, (*target)->TaskIterator());
        }

        // Tasks
        out << ",\"" << (*target)->Name << "\":";
        NStatsFormat::FormatJSON(states, out);
    }

    out << ",\"#\":" << LastRevision << "}";

    if (!jsonp.empty())
        out << ");";
}
