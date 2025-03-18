#include "http.h"
#include "http_common.h"
#include "log.h"
#include "master.h"
#include "master_target_graph.h"
#include "messages.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/util.h>

#include <library/cpp/xml/encode/encodexml.h>

#include <util/generic/algorithm.h>
#include <util/generic/set.h>

void TMasterHttpRequest::ServeVariables(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    TLockableHandle<TMasterGraph>::TReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReadLock pool(GetPool());

    PrintSimpleHttpHeader(out, "text/html");

    FormatHeader(out, PT_VARIABLES);

    out << "<h1>Worker variables</h1>";

    if (!ReadOnly) {
        out << "All hosts: <a id=\"setVariable\">set variable</a> / <a id=\"unsetVariable\">unset variable</a>, ";
        out << "selected hosts: <a id=\"setVariableOnSelectedHosts\">set variable</a> / <a id=\"unsetVariableOnSelectedHosts\">unset variable</a>, ";
        out << "master: <a id=\"setMasterVariable\">set variable</a> / <a id=\"unsetMasterVariable\">unset variable</a>.";

        out << "<br><br>";
    }

    TSet<TString> allVariables;

    TLockableHandle<TVariablesMap>::TReadLock variables(graph->GetVariables());
    if (variables->size() > 0) {
        out << "<p>Master variables</p>";
        out << "<table class=\"grid vars\" urlroot=\"" << UrlRoot << "\">";
        out << "<thead><tr>";
        for (const auto& var : *variables) {
            out << "<th>" << var.first << "</th>";
        }
        out << "</tr></thead>";

        out << "<tbody><tr>";
        for (const auto& var : *variables) {
            out << "<td>" << var.second << "</td>";
        }
        out << "</tr></tbody></table><br><br>";
    }

    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker)
        for (TWorkerVariablesLight::TMapType::const_iterator variable = (*worker)->GetVariables().GetMap().begin(); variable != (*worker)->GetVariables().GetMap().end(); ++variable)
            allVariables.insert(variable->Name);

    if (variables->size() > 0) {
        out << "Workers variables";
    }
    out << "<table class=\"grid vars\" id=\"MasterElement\" urlroot=\"" << UrlRoot << "\">";

    out << "<script type=\"application/json\" id=\"strongDefaultVars\">{";
    bool first = true;
    for (TMasterGraph::TVariablesMap::const_iterator i = graph->GetStrongVariables().begin(); i != graph->GetStrongVariables().end(); ++i) {
        if (!first)
            out << ",";
        out << "\"" << i->first << "\":\"strong\"";
        first = false;
    }
    for (TMasterGraph::TVariablesMap::const_iterator i = graph->GetDefaultVariables().begin(); i != graph->GetDefaultVariables().end(); ++i) {
        if (!first)
            out << ",";
        out << "\"" << i->first << "\":\"default\"";
        first = false;
    }
    out << "}</script>";

    out << "<thead><tr id=\"headerRow\">";
    out << "<th style=\"text-align:right\"><span style=\"vertical-align:sub\">Hosts</span> <span style=\"vertical-align:super\">Variables</span></th>";
    out << "</tr></thead>";

    out << "<tbody id=\"varsList\">";

    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
        const TString& workerName = (*worker)->GetHost();

        out << "<tr name=\"@@\" worker=\"" << workerName << "\">";

        out << "<th>";
        if (!ReadOnly) {
            out << "<label><input type=\"checkbox\" name=\"varsHost\" value=\"" << workerName << "\"/>" << workerName << "</label>";
        } else {
            out << workerName;
        }
        out << "</th>";

        out << "</tr>";
    }

    out << "</table>";

    out.Flush();
}

void TMasterHttpRequest::ServeVariablesCommand(IOutputStream& out, const TString& cmd) {
    TVariablesMessage::EVariableAction action = TVariablesMessage::VA_SET;
    TString name, value, worker;
    bool master = false;

    if (cmd == "set") {
        action = TVariablesMessage::VA_SET;
    } else if (cmd == "unset") {
        action = TVariablesMessage::VA_UNSET;
    } else {
        return ServeSimpleStatus(out, HTTP_BAD_REQUEST);
    }

    for (TQueryMap::iterator i = Query.begin(); i != Query.end(); ++i) {
        if (i->first == "name") {
            name = i->second;
        } else if (i->first == "value") {
            value = i->second;
        } else if (i->first == "worker") {
            worker = i->second;
        } else if (i->first == "master") {
            master = true;
        }
    }

    {
        const bool isValidName = AllOf(name, [](char c) {
            return isalnum(c) || c == '_' || c == '.';
        });
        if (name.size() == 0 || !isValidName) {
            return ServeSimpleStatus(out, HTTP_BAD_REQUEST);
        }
    }

    if (master) {
        TLockableHandle<TMasterGraph>::TWriteLock graph(GetGraph());
        if (action == TVariablesMessage::VA_SET) {
            graph->AddVariable(name, value);
        }
        else {
            graph->DeleteVariable(name);
        }
        return ServeSimpleStatus(out, HTTP_NO_CONTENT);
    }

    // do not allow to change forced variables
    {
        TLockableHandle<TMasterGraph>::TReadLock graph(GetGraph());

        if (graph->GetStrongVariables().find(name) != graph->GetStrongVariables().end())
            return ServeSimpleStatus(out, HTTP_BAD_REQUEST);
    }

    TVariablesMessage msg(name, value, action);

    DEBUGLOG1(http, RequesterName << ": Variables command (" << msg.FormatText() << ")(" << worker << "), enqueuing");

    if (!worker.empty()) {
        TLockableHandle<TWorkerPool>::TWriteLock(GetPool())->EnqueueMessageToWorker(worker, msg);
    } else {
        TLockableHandle<TWorkerPool>::TWriteLock(GetPool())->EnqueueMessageAll(msg);
    }

    return ServeSimpleStatus(out, HTTP_NO_CONTENT);
}

void TMasterHttpRequest::ServeVariableStatusText(IOutputStream& out0, const TString& name) {
    TBufferedOutput out(&out0);

    TLockableHandle<TWorkerPool>::TReadLock pool(GetPool());

    TSet<TString> values;

    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
        TWorkerVariablesLight::TMapType::const_iterator variable = (*worker)->GetVariables().GetMap().find(name);
        if (variable != (*worker)->GetVariables().GetMap().end())
            values.insert(variable->Value);
        else
            values.insert("UNSET"); // Not good. There could be ordinary value 'UNSET' but I have no other option - there is no way to
                // distinguish special and ordinary variable value
    }

    PrintSimpleHttpHeader(out, "text/plain");

    for (TSet<TString>::const_iterator val = values.begin(); val != values.end(); ++val) {
        if (val != values.begin())
            out << ",";
        out << *val;
    }

    out.Flush();
}

void TMasterHttpRequest::ServeVariableStatus(IOutputStream& out0) {
    TBufferedOutput out(&out0);

    TLockableHandle<TWorkerPool>::TReadLock pool(GetPool());

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

    if (!jsonp.empty())
        out << jsonp << "(";

    out << "{\"workers\":{";
    bool first = true;
    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
        LastRevision = Max(LastRevision, (*worker)->GetVariablesRevision());

        if ((*worker)->GetVariablesRevision() <= Revision)
            continue;

        if (!first) {
            out << ",";
        }
        out << "\"" << (*worker)->GetHost() << "\":{";
        out << "\"status\":\"" << ToString((*worker)->GetState()) << "\",";
        out << "\"variables\":{";

        const TWorkerVariablesLight::TMapType variables = (*worker)->GetVariables().GetMap();
        for (TWorkerVariablesLight::TMapType::const_iterator variable = variables.begin(); variable != variables.end(); ++variable) {
            if (variable != variables.begin())
                out << ",";
            out << "\"" << variable->Name << "\":\"" << EscapeC(variable->Value) << "\"";
        }

        out << "}";
        out << "}";

        first = false;
    }
    out << "},";
    out << "\"#\":" << LastRevision;
    out << "}";

    if (!jsonp.empty())
        out << ");";

    out.Flush();
}
